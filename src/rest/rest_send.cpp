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


restHandler rest_handler; // Global data sender


restHandler::restHandler() {
    bool send_full_config_ticker = false;
    bool send_lcd_ticker = false;
    bool register_device_ticker = false;
}

void restHandler::init()
{
    // Set up timers
    registerDeviceTicker.once(5, [](){rest_handler.register_device_ticker = true;});
    lcdTicker.once(25, [](){rest_handler.send_lcd_ticker = true;});
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
    send_full_config();
    register_device();
    send_lcd();
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

    if (WiFiClass::status() != WL_CONNECTED) {
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
    WiFiClientFixed client;
    if(true) {
        // client.setInsecure();
        {
            // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
            HTTPClient http;

            http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
            http.setConnectTimeout(6000);
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


bool restHandler::send_bluetooth_crash_report() {
    String payload;
    {
        DynamicJsonDocument doc(1024);
        const char *url;
        char guid[20];

        getGuid(guid);


        doc["uptime"] = esp_timer_get_time();
        doc["device_id"] = guid;

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
    else
        send_full_config_ticker = false;

    fullConfigTicker.detach();
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
        doc["devices"] = devices.as<JsonObject>();

        doc["uptime"] = esp_timer_get_time();

        doc[UpstreamSettingsKeys::deviceID] = upstreamSettings.deviceID;
        doc[UpstreamSettingsKeys::apiKey] = upstreamSettings.apiKey;

        // Serialize the JSON document
        serializeJson(doc, payload);
    }
    
    send_json_str(payload, url, httpMethod::HTTP_PUT);
    return true;
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

    // If we've already registered or are missing critical information necessary to register, skip this attempt and reset the timer
    if(upstreamSettings.isRegistered() || strlen(upstreamSettings.username) == 0 || strlen(upstreamSettings.upstreamHost) == 0)
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
                send_lcd_ticker = true;
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



bool restHandler::send_lcd() {
    String payload;
    char url[256] = "";

    // Only send if the semaphore is set - otherwise return
    if(!send_lcd_ticker)
        return false;
    else
        send_lcd_ticker = false;

    lcdTicker.detach();
    lcdTicker.once(LCD_PUSH_DELAY, [](){rest_handler.send_lcd_ticker = true;});

    if(upstreamSettings.isRegistered() == false)
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::LCD))
        return false;

    {
        DynamicJsonDocument doc(1024);
        DynamicJsonDocument lcd(512);
        getLcdContentJson(lcd);

        doc[UpstreamSettingsKeys::deviceID] = upstreamSettings.deviceID;
        doc[UpstreamSettingsKeys::username] = upstreamSettings.username;
        doc["lcd"] = lcd;

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, url, httpMethod::HTTP_PUT);

    return true;
}
