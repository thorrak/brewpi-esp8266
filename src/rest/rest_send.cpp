#include <Arduino.h>
#include <ArduinoJson.h>
#include <ctime>
#include "Ticker.h"
// #define LCBURL_MDNS
// #include <LCBUrl.h>
#include <ArduinoLog.h>

#include "rest_send.h"
#include "http_server.h"
#include "Version.h"
#include "Config.h"
#include "getGuid.h"
#include "EepromManager.h"
#include "TempControl.h"
#include "DeviceManager.h"
#include "JsonMessages.h"
#include "JsonKeys.h"
#include "SettingLoader.h"


restHandler rest_handler; // Global data sender


restHandler::restHandler() {
    send_full_config_ticker = false;
    send_status_ticker = false;
    register_device_ticker = false;
    messages_pending_on_server = false;
}

void restHandler::init()
{
    // Set up timers
    registerDeviceTicker.once(5, [](){rest_handler.register_device_ticker = true;});
    statusTicker.once(25, [](){rest_handler.send_status_ticker = true;});
    fullConfigTicker.once(15, [](){rest_handler.send_full_config_ticker=true;});
}


void restHandler::get_useragent(char *ua, size_t size) {
    snprintf(ua, size,
        "brewpi-esp/%s (commit %s)",
        FIRMWARE_REVISION,
        Config::Version::git_tag
    );
}


void restHandler::process() {
    register_device();
    send_status();
    get_messages(false);
    process_messages();
    send_full_config();
}

sendResult restHandler::send_json_str(String &payload, const char *url, httpMethod method) {
    String response;
    return send_json_str(payload, url, response, method);
}

sendResult restHandler::send_json_str(String &payload, const char *url, String &response, httpMethod method) {
    char auth_header[64];
    char userAgent[128];
    int httpResponseCode;
    sendResult result;

    send_lock = true;


#ifdef ESP8266
    if (WiFi.status() != WL_CONNECTED) {
#else
    if (WiFiClass::status() != WL_CONNECTED) {
#endif
        Serial.print(F("send_json_str: Wifi not connected, skipping send.\r\n"));
        send_lock = false;
        return sendResult::retry;
    }

    get_useragent(userAgent, sizeof(userAgent));

    // snprintf(auth_header, sizeof(auth_header), "token %s", config.secret);
   
    // Log.verbose(F("send_json_str: Sending %s to %s\r\n"), payload.c_str(), url);
    Serial.printf("send_json_str: Sending %s to %s\r\n", payload.c_str(), url);

    yield();  // Yield before we lock up the radio

    // TODO - Determine if we can get rid of the call to new
    // WiFiClientSecure *client = new WiFiClientSecure;
#ifdef ESP8266
    WiFiClient client;
#else
    WiFiClientFixed client;
#endif
    if(true) {
        // client.setInsecure();
        {
            // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
            HTTPClient http;

            http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
#ifndef ESP8266
            http.setConnectTimeout(6000);
#endif
            http.setReuse(false);

            if (http.begin(client, url)) {
                http.addHeader(F("Content-Type"), F("application/json"));
                // http.addHeader(F("Authorization"), auth_header);
                http.setUserAgent(userAgent);

                // Use whatever method we were passed
                httpResponseCode = http.sendRequest(httpMethodToString(method), payload);

                response = http.getString();

                if (httpResponseCode < HTTP_CODE_OK || httpResponseCode > HTTP_CODE_NO_CONTENT) {
                    Serial.printf("send_json_str: Send failed (%d): %s. Response:\r\n%s\r\n",
                        httpResponseCode,
                        http.errorToString(httpResponseCode).c_str(),
                        http.getString().c_str());
                    // Log.error(F("send_json_str: Send failed (%d): %s. Response:\r\n%s\r\n"),
                    //     httpResponseCode,
                    //     http.errorToString(httpResponseCode).c_str(),
                    //     http.getString().c_str());
                    result = sendResult::failure;
                } else {
                    Serial.print(F("send_json_str: success!\r\n"));
                    // Log.verbose(F("send_json_str: success!\r\n"));
                    // Log.verbose(F("send_json_str: Response:\r\n%s\r\n"),
                    //     http.getString().c_str());
                    result = sendResult::success;
                }
                http.end();
            } else {
                Serial.print(F("send_json_str: Unable to create connection\r\n"));
                // Log.error(F("send_json_str: Unable to create connection\r\n"));
                result = sendResult::failure;
            }
        }
        // delete client;
    }

    send_lock = false;
    return result;
}


bool restHandler::get_url(char *url, size_t size, const char *path) {
    if(strlen(upstreamSettings.upstreamHost) <= 3) {
        Serial.print(F("get_url: No upstream host configured, should skip send.\r\n"));
        return false;
    } else if(upstreamSettings.upstreamPort <= 0 || upstreamSettings.upstreamPort > 65535) {
        Serial.print(F("get_url: No upstream port configured, should skip send.\r\n"));
        return false;
    }

    if(upstreamSettings.upstreamPort == 80) {
        snprintf(url, size, "http://%s%s", upstreamSettings.upstreamHost, path);
    } else {
        snprintf(url, size, "http://%s:%d%s", upstreamSettings.upstreamHost, upstreamSettings.upstreamPort, path);
    }
    return true;
}

bool restHandler::get_url(char *url, size_t size, const char *path, const char *device_id, const char *api_key) {
    // Used when we need to send the device ID and API key as part of the URL (HTTP_GET)
    if(!get_url(url, size, path))
        return false;
    
    // Ensure the buffer is large enough for the base URL plus the additional parameters
    size_t base_url_length = strlen(url);
    if (base_url_length + strlen(UpstreamSettingsKeys::deviceID) + strlen(device_id) + strlen(UpstreamSettingsKeys::apiKey) + strlen(api_key) + 10 > size) {
        // Handle error: buffer not large enough
        return false;
    }

    // Use a temporary buffer to format the URL with parameters
    char temp_url[size];
    snprintf(temp_url, size, "%s?%s=%s&%s=%s", url, UpstreamSettingsKeys::deviceID, device_id, UpstreamSettingsKeys::apiKey, api_key);
    
    // Copy the formatted URL back into the original buffer
    strncpy(url, temp_url, size);

    return true;
}


bool restHandler::send_bluetooth_crash_report() {
    String payload;
    {
        DynamicJsonDocument doc(1024);
        const char *url;
        char guid[20];

        getGuid(guid);


#ifdef ESP8266
        doc["uptime"] = system_get_time();
#else
        doc["uptime"] = esp_timer_get_time();
#endif
        doc["device_id"] = guid;
        doc["message"] = "With Arduino 2.0.9";

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, "http://www.fermentrack.com/api/bluetooth_crash/", httpMethod::HTTP_POST);
    return true;
    
}

bool restHandler::send_full_config() {
    char url[256] = "";
    String payload;

    // Only send if the semaphore is set - otherwise return
    if(!send_full_config_ticker)
        return false;

    // Force getting messages before sending the full config
    // We do this here, before setting send_full_config_ticker false, as get_messages() will set it to true if there are config updates.
    // We can just send them as part of this full config push instead of having to queue up a second
    fullConfigTicker.detach();
    get_messages(true);
    send_full_config_ticker = false;
    fullConfigTicker.once(FULL_CONFIG_PUSH_DELAY, [](){rest_handler.send_full_config_ticker=true;});

    if(!upstreamSettings.isRegistered())
        return false;  // If we aren't registered, we have nowhere to send the data
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::fullConfig))
        return false;  // Skip send if the URL is not set

    {
#if !defined(HAS_BLUETOOTH) && !defined(EXTERN_SENSOR_ACTUATOR_SUPPORT)
        DynamicJsonDocument doc(8192);
        DynamicJsonDocument devices(2048);
#else
        DynamicJsonDocument doc(8192*2);
        DynamicJsonDocument devices(8192);
#endif
        DynamicJsonDocument cs(256);
        DynamicJsonDocument cc(1024);
        DynamicJsonDocument cv(1024);
        DynamicJsonDocument es(256);
        DynamicJsonDocument mt(1024);

        tempControl.getControlSettingsDoc(cs);
        tempControl.getControlConstantsDoc(cc);
        tempControl.getControlVariablesDoc(cv);
        extendedSettings.toJson(es);
        minTimes.toJson(mt);

        EnumerateHardware spec;
        spec.values = 0;  // Change if we want to poll values here as well
        deviceManager.enumerateHardware(devices, spec);

        doc["cs"] = cs.as<JsonObject>();
        doc["cc"] = cc.as<JsonObject>();
        doc["cv"] = cv.as<JsonObject>();
        doc["es"] = es.as<JsonObject>();
        doc["mt"] = mt.as<JsonObject>();
        doc["devices"] = devices.as<JsonArray>();

#ifdef ESP8266
        doc["uptime"] = system_get_time();
#else
        doc["uptime"] = esp_timer_get_time();
#endif

        doc[UpstreamSettingsKeys::deviceID] = upstreamSettings.deviceID;
        doc[UpstreamSettingsKeys::apiKey] = upstreamSettings.apiKey;

        doc[UpstreamSettingsKeys::firmwareRelease] = Config::Version::release;
        doc[UpstreamSettingsKeys::firmwareRevision] = Config::Version::git_rev;
        doc[UpstreamSettingsKeys::firmwareTag] = Config::Version::git_tag;
        doc[UpstreamSettingsKeys::firmwareVersion] = FIRMWARE_REVISION;

        // Serialize the JSON document
        serializeJson(doc, payload);
    }
    
    send_json_str(payload, url, httpMethod::HTTP_PUT);
    return true;
}

bool restHandler::configured_for_fermentrack_rest() {
    if(upstreamSettings.isRegistered())
        return true;  // If we're registered, we're obviously configured
    if(strlen(upstreamSettings.username) == 0 && strlen(upstreamSettings.apiKey) == 0)
        return false; 
    if(strlen(upstreamSettings.upstreamHost) <= 3 || (upstreamSettings.upstreamPort <= 0 || upstreamSettings.upstreamPort > 65535))
        return false;

    return true;  // We have a username/apiKey and a valid host/port, but aren't registered yet. Clearly the user wants to use fermentrack_rest
}


bool restHandler::register_device() {
    char url[256] = "";
    String payload;
    String response;

    // Only send if the semaphore is set - otherwise return
    if(!register_device_ticker)
        return false;
    else
        register_device_ticker = false;

    registerDeviceTicker.detach();
    registerDeviceTicker.once(REGISTER_DEVICE_DELAY, [](){rest_handler.register_device_ticker = true;});

    // If we've already registered or are missing critical information necessary to register, skip this attempt
    if(upstreamSettings.isRegistered() || (strlen(upstreamSettings.username) == 0 && strlen(upstreamSettings.apiKey) == 0))
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::registerDevice))
        return false;   // Skip send if the URL is not set

    {
        DynamicJsonDocument doc(1024);

        char guid[20];
        getGuid(guid);

        char hw_str[2];
        hw_str[0] = BREWPI_BOARD;
        hw_str[1] = '\0';

        doc["guid"] = guid;
        if(strlen(upstreamSettings.username) > 0)
            doc[UpstreamSettingsKeys::username] = upstreamSettings.username;
        else
            doc[UpstreamSettingsKeys::apiKey] = upstreamSettings.apiKey;
        doc["hardware"] = hw_str;
        doc["version"] = FIRMWARE_REVISION;

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, url, response, httpMethod::HTTP_PUT);

    {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);


        // response = {'success': True, 
        // 'message': 'Device registered', 
        // 'msg_code': 0, 
        // 'device_id': device.id, 
        // 'created': created}
        if(doc.containsKey("success") && doc["success"].as<bool>()) {
            bool success = doc["success"].as<bool>();

            if(success) {
                // We successfully set the device ID & API key
                upstreamSettings.upstreamRegistrationError = UpstreamSettings::upstreamRegErrorT::NO_ERROR;
                strlcpy(upstreamSettings.deviceID, doc[UpstreamSettingsKeys::deviceID].as<const char *>(), sizeof(upstreamSettings.deviceID));
                strlcpy(upstreamSettings.apiKey, doc[UpstreamSettingsKeys::apiKey].as<const char *>(), sizeof(upstreamSettings.apiKey));
                upstreamSettings.username[0] = '\0';  // Clear the username since we now have the apiKey

                // Store the updated settings
                upstreamSettings.storeToSpiffs(); 

                // Also, trigger sends
                send_status_ticker = true;
                send_full_config_ticker = true;

            } else {
                // We didn't set the device ID (were unable to register). Set an error code.
                upstreamSettings.upstreamRegistrationError = (UpstreamSettings::upstreamRegErrorT) doc["msg_code"].as<uint8_t>();
            }
	    } else {
            // Invalid response
            upstreamSettings.upstreamRegistrationError = UpstreamSettings::upstreamRegErrorT::REGISTRATION_ENDPOINT_ERR;
        } 

    }

    return true;
}



bool restHandler::send_status() {
    String payload;
    char url[256] = "";
    String response;

    // Only send if the semaphore is set - otherwise return
    if(!send_status_ticker)
        return false;
    else
        send_status_ticker = false;

    statusTicker.detach();
    statusTicker.once(LCD_PUSH_DELAY, [](){rest_handler.send_status_ticker = true;});

    if(upstreamSettings.isRegistered() == false)
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::status))
        return false;

    {
        DynamicJsonDocument doc(1536);
        DynamicJsonDocument lcd(512);
        DynamicJsonDocument temps(512);

        getLcdContentJson(lcd);
        printTemperaturesJson(temps, "", "", true);

        doc[UpstreamSettingsKeys::deviceID] = upstreamSettings.deviceID;
        doc[UpstreamSettingsKeys::apiKey] = upstreamSettings.apiKey;
        doc["lcd"] = lcd;
        doc["temps"] = temps;
        doc["temp_format"] = String(tempControl.cc.tempFormat);
        doc["mode"] = String(tempControl.cs.mode);

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, url, response, httpMethod::HTTP_PUT);

    // Check if we have any messages pending on the server, and set the flag if so
    {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        if(doc.containsKey("has_messages") && doc["has_messages"].as<bool>()) {
            bool has_messages = doc["has_messages"].as<bool>();

            if(has_messages)
                messages_pending_on_server = true;
        }

        if(doc.containsKey("updated_mode") && doc["updated_mode"].as<const char *>()) {
            char updated_mode = doc["updated_mode"].as<const char *>()[0];

            if (updated_mode == Modes::fridgeConstant || updated_mode == Modes::beerConstant || updated_mode == Modes::beerProfile ||
                updated_mode == Modes::off || updated_mode == Modes::test) {
                    // We have a new, valid mode. Update to it.
                    if(tempControl.cs.mode != updated_mode) {
                        Serial.printf("Updating to valid mode \"%c\" (0x%02X)\r\n", updated_mode, updated_mode);
                        tempControl.setMode(updated_mode);
                        send_status_ticker = true;  // Trigger a send to update the LCD
                    }
            } else {
                Serial.printf("Invalid mode \"%c\" (0x%02X)\r\n", updated_mode, updated_mode);
            }
        }

        if(doc.containsKey("updated_setpoint") && doc["updated_setpoint"].as<const char *>()) {
            switch(tempControl.cs.mode) {
                case Modes::fridgeConstant:
                    SettingLoader::setFridgeSetting(doc["updated_setpoint"].as<const char *>());
                    send_status_ticker = true;  // Trigger a send to update the LCD
                    break;
                case Modes::beerConstant:
                case Modes::beerProfile:
                    SettingLoader::setBeerSetting(doc["updated_setpoint"].as<const char *>());
                    Serial.printf("Received updated setpoint \"%s\"\r\n", doc["updated_setpoint"].as<const char *>());
                    send_status_ticker = true;  // Trigger a send to update the LCD
                    break;
                default:
                    break;
            }
        }


    }

    return true;
}



bool restHandler::get_messages(bool override=false) {
    String payload = "";
    char url[256] = "";
    String response;

    // Only retrieve if we're being forced (via override) or if there are messages on the server
    if(!messages_pending_on_server && !override)
        return false;

    // We can't retrieve messages if we're not registered
    if(upstreamSettings.isRegistered() == false)
        return false;
    // Since this endpoint uses get, we have to add the device ID and apiKey to the URL
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::messages, upstreamSettings.deviceID, upstreamSettings.apiKey))
        return false;

    // {
    //     DynamicJsonDocument doc(512);

    //     doc[UpstreamSettingsKeys::deviceID] = upstreamSettings.deviceID;
    //     doc[UpstreamSettingsKeys::apiKey] = upstreamSettings.apiKey;

    //     // Serialize the JSON document
    //     serializeJson(doc, payload);
    // }

    send_json_str(payload, url, response, httpMethod::HTTP_GET);

    // Parse any messages that are on the server
    {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        // Process any flags on the server
        if(doc.containsKey(RestMessagesKeys::messages)) {
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::updated_cs) && doc[RestMessagesKeys::messages][RestMessagesKeys::updated_cs].as<bool>())
                messages.updated_cs = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_cs].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::updated_cc) && doc[RestMessagesKeys::messages][RestMessagesKeys::updated_cc].as<bool>())
                messages.updated_cc = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_cc].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::updated_mt) && doc[RestMessagesKeys::messages][RestMessagesKeys::updated_mt].as<bool>())
                messages.updated_mt = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_mt].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::updated_es) && doc[RestMessagesKeys::messages][RestMessagesKeys::updated_es].as<bool>())
                messages.updated_es = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_es].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::updated_devices) && doc[RestMessagesKeys::messages][RestMessagesKeys::updated_devices].as<bool>())
                messages.updated_devices = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_devices].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::default_cc) && doc[RestMessagesKeys::messages][RestMessagesKeys::default_cc].as<bool>())
                messages.default_cc = doc[RestMessagesKeys::messages][RestMessagesKeys::default_cc].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::default_cs) && doc[RestMessagesKeys::messages][RestMessagesKeys::default_cs].as<bool>())
                messages.default_cs = doc[RestMessagesKeys::messages][RestMessagesKeys::default_cs].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::reset_eeprom) && doc[RestMessagesKeys::messages][RestMessagesKeys::reset_eeprom].as<bool>())
                messages.reset_eeprom = doc[RestMessagesKeys::messages][RestMessagesKeys::reset_eeprom].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::reset_connection) && doc[RestMessagesKeys::messages][RestMessagesKeys::reset_connection].as<bool>())
                messages.reset_connection = doc[RestMessagesKeys::messages][RestMessagesKeys::reset_connection].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::restart_device) && doc[RestMessagesKeys::messages][RestMessagesKeys::restart_device].as<bool>())
                messages.restart_device = doc[RestMessagesKeys::messages][RestMessagesKeys::restart_device].as<bool>();
            if(doc[RestMessagesKeys::messages].containsKey(RestMessagesKeys::refresh_config) && doc[RestMessagesKeys::messages][RestMessagesKeys::refresh_config].as<bool>())
                messages.refresh_config = doc[RestMessagesKeys::messages][RestMessagesKeys::refresh_config].as<bool>();
        }
     
    }

    messages_pending_on_server = false;  // TODO - Remove once done with testing, as we should only unset this if we have a confirmed message count of 0
    return true;
}

bool restHandler::set_message_processed(const char* message_type_key) {
    String payload = "";
    char url[256] = "";
    String response;

    // We can't delete messages if we're not registered
    if(upstreamSettings.isRegistered() == false)
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::messages))
        return false;

    {
        DynamicJsonDocument doc(512);

        doc[UpstreamSettingsKeys::deviceID] = upstreamSettings.deviceID;
        doc[UpstreamSettingsKeys::apiKey] = upstreamSettings.apiKey;
        doc[message_type_key] = false;

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, url, response, httpMethod::HTTP_PATCH);

    // TODO - parse the response and make sure it was successful

    return true;
}

