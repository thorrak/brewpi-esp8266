#include <ArduinoJson.h>
#include <string>
#include <ctime>

#include <thorlog.h>
#include <thorlog_espidf.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

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

#ifdef HAS_BLUETOOTH
#include "wireless/BTScanner.h"
#endif


// Context for capturing HTTP response body via esp_http_client event handler
struct HttpResponseCtx {
    char* buffer;
    size_t buffer_size;
    size_t bytes_received;
};

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    HttpResponseCtx* ctx = (HttpResponseCtx*)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && ctx && ctx->buffer) {
        size_t space = ctx->buffer_size - ctx->bytes_received - 1;
        size_t copy = (evt->data_len < space) ? evt->data_len : space;
        memcpy(ctx->buffer + ctx->bytes_received, evt->data, copy);
        ctx->bytes_received += copy;
        ctx->buffer[ctx->bytes_received] = '\0';
    }
    return ESP_OK;
}

restHandler rest_handler; // Global data sender


// FreeRTOS timer callbacks
static void fullConfigTimerCallback(TimerHandle_t xTimer) {
    rest_handler.send_full_config_ticker = true;
}

static void statusTimerCallback(TimerHandle_t xTimer) {
    rest_handler.send_status_ticker = true;
}

static void registerDeviceTimerCallback(TimerHandle_t xTimer) {
    rest_handler.register_device_ticker = true;
}


restHandler::restHandler() {
    send_full_config_ticker = false;
    send_status_ticker = false;
    register_device_ticker = false;
    messages_pending_on_server = false;
    pendingDeviceName[0] = '\0';
}

void restHandler::init()
{
    // Create one-shot FreeRTOS software timers
    fullConfigTicker = xTimerCreate("fullCfg", pdMS_TO_TICKS(15 * 1000), pdFALSE, nullptr, fullConfigTimerCallback);
    statusTicker = xTimerCreate("status", pdMS_TO_TICKS(25 * 1000), pdFALSE, nullptr, statusTimerCallback);
    registerDeviceTicker = xTimerCreate("regDev", pdMS_TO_TICKS(5 * 1000), pdFALSE, nullptr, registerDeviceTimerCallback);

    // Start all three timers
    xTimerStart(fullConfigTicker, 0);
    xTimerStart(statusTicker, 0);
    xTimerStart(registerDeviceTicker, 0);
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

sendResult restHandler::send_json_str(std::string &payload, const char *url, httpMethod method) {
    std::string response;
    return send_json_str(payload, url, response, method);
}

sendResult restHandler::send_json_str(std::string &payload, const char *url, std::string &response, httpMethod method) {
    char userAgent[128];
    sendResult result;

    send_lock = true;

    if (!bp_wifi_is_connected()) {
        Log.warning("send_json_str: Wifi not connected, skipping send.\r\n");
        send_lock = false;
        return sendResult::retry;
    }

    get_useragent(userAgent, sizeof(userAgent));

    Log.info("send_json_str: Sending %s to %s\r\n", payload.c_str(), url);

    vTaskDelay(pdMS_TO_TICKS(1));  // Yield before we lock up the radio

    // Buffer for capturing the HTTP response body
    static constexpr size_t RESPONSE_BUF_SIZE = 2048;
    char response_buf[RESPONSE_BUF_SIZE];
    HttpResponseCtx response_ctx = { response_buf, RESPONSE_BUF_SIZE, 0 };

    // Map our internal httpMethod enum to esp_http_client method
    esp_http_client_method_t esp_method;
    switch (method) {
        case httpMethod::HTTP_PUT:    esp_method = HTTP_METHOD_PUT;    break;
        case httpMethod::HTTP_POST:   esp_method = HTTP_METHOD_POST;   break;
        case httpMethod::HTTP_PATCH:  esp_method = HTTP_METHOD_PATCH;  break;
        case httpMethod::HTTP_DELETE: esp_method = HTTP_METHOD_DELETE; break;
        case httpMethod::HTTP_GET:
        default:                      esp_method = HTTP_METHOD_GET;    break;
    }

    esp_http_client_config_t config = {};
    config.url = url;
    config.method = esp_method;
    config.event_handler = http_event_handler;
    config.user_data = &response_ctx;
    config.timeout_ms = 6000;
    config.disable_auto_redirect = false;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        Log.error("send_json_str: Unable to create esp_http_client\r\n");
        send_lock = false;
        return sendResult::failure;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "User-Agent", userAgent);

    if (esp_method != HTTP_METHOD_GET && !payload.empty()) {
        esp_http_client_set_post_field(client, payload.c_str(), payload.length());
    }

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        response.assign(response_buf, response_ctx.bytes_received);

        if (status_code < 200 || status_code > 204) {
            Log.error("send_json_str: Send failed (%d). Response:\r\n%s\r\n",
                status_code, response_buf);
            result = sendResult::failure;
        } else {
            Log.info("send_json_str: success!\r\n");
            result = sendResult::success;
        }
    } else {
        Log.error("send_json_str: HTTP request failed: %s\r\n", esp_err_to_name(err));
        result = sendResult::failure;
    }

    esp_http_client_cleanup(client);

    send_lock = false;
    return result;
}


bool restHandler::get_url(char *url, size_t size, const char *path) {
    if(strlen(upstreamSettings.upstreamHost) <= 3) {
        Log.error("get_url: No upstream host configured, should skip send.\r\n");
        return false;
    } else if(upstreamSettings.upstreamPort <= 0 || upstreamSettings.upstreamPort > 65535) {
        Log.error("get_url: No upstream port configured, should skip send.\r\n");
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
    std::string payload;
    {
        JsonDocument doc;
        char guid[20];

        getGuid(guid);

        doc["uptime"] = esp_timer_get_time();
        doc["device_id"] = guid;
        doc["message"] = "With Arduino 3.0.4";

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, "http://www.fermentrack.com/api/bluetooth_crash/", httpMethod::HTTP_POST);
    return true;
    
}

bool restHandler::send_full_config() {
    char url[256] = "";
    std::string payload;

    // Only send if the semaphore is set - otherwise return
    if(!send_full_config_ticker)
        return false;

    // Force getting messages before sending the full config
    // We do this here, before setting send_full_config_ticker false, as get_messages() will set it to true if there are config updates.
    // We can just send them as part of this full config push instead of having to queue up a second
    xTimerStop(fullConfigTicker, 0);
    get_messages(true);
    send_full_config_ticker = false;
    xTimerChangePeriod(fullConfigTicker, pdMS_TO_TICKS(FULL_CONFIG_PUSH_DELAY * 1000), 0);

    if(!upstreamSettings.isRegistered())
        return false;  // If we aren't registered, we have nowhere to send the data
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::fullConfig))
        return false;  // Skip send if the URL is not set

    {
        JsonDocument doc;
        JsonDocument devices;
        JsonDocument cs;
        JsonDocument cc;
        JsonDocument cv;
        JsonDocument es;
        JsonDocument mt;

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
        doc["uptime"] = esp_timer_get_time();

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
    std::string payload;
    std::string response;

    // Only send if the semaphore is set - otherwise return
    if(!register_device_ticker)
        return false;
    else
        register_device_ticker = false;

    xTimerStop(registerDeviceTicker, 0);
    xTimerChangePeriod(registerDeviceTicker, pdMS_TO_TICKS(REGISTER_DEVICE_DELAY * 1000), 0);

    // If we've already registered or are missing critical information necessary to register, skip this attempt
    if(upstreamSettings.isRegistered() || (strlen(upstreamSettings.username) == 0 && strlen(upstreamSettings.apiKey) == 0))
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::registerDevice))
        return false;   // Skip send if the URL is not set

    {
        JsonDocument doc;

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
        if(strlen(pendingDeviceName) > 0)
            doc[UpstreamSettingsKeys::deviceName] = pendingDeviceName;

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, url, response, httpMethod::HTTP_PUT);

    {
        JsonDocument doc;
        deserializeJson(doc, response);


        // response = {'success': True, 
        // 'message': 'Device registered', 
        // 'msg_code': 0, 
        // 'device_id': device.id, 
        // 'created': created}
        if(doc["success"].is<bool>() && doc["success"].as<bool>()) {
            bool success = doc["success"].as<bool>();

            if(success) {
                // We successfully set the device ID & API key
                upstreamSettings.upstreamRegistrationError = UpstreamSettings::upstreamRegErrorT::NO_ERROR;
                strlcpy(upstreamSettings.deviceID, doc[UpstreamSettingsKeys::deviceID].as<const char *>(), sizeof(upstreamSettings.deviceID));
                strlcpy(upstreamSettings.apiKey, doc[UpstreamSettingsKeys::apiKey].as<const char *>(), sizeof(upstreamSettings.apiKey));
                upstreamSettings.username[0] = '\0';  // Clear the username since we now have the apiKey
                pendingDeviceName[0] = '\0';  // Clear the pending device name

                // Store the updated settings
                upstreamSettings.storeToFilesystem(); 

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
    std::string payload;
    char url[256] = "";
    std::string response;

    // Only send if the semaphore is set - otherwise return
    if(!send_status_ticker)
        return false;
    else
        send_status_ticker = false;

    xTimerStop(statusTicker, 0);
    xTimerChangePeriod(statusTicker, pdMS_TO_TICKS(LCD_PUSH_DELAY * 1000), 0);

    if(upstreamSettings.isRegistered() == false)
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::status))
        return false;

    {
        JsonDocument doc;
        JsonDocument lcd;
        JsonDocument temps;

        getLcdContentJson(lcd);
        printTemperaturesJson(temps, "", "", true);

        doc[UpstreamSettingsKeys::deviceID] = upstreamSettings.deviceID;
        doc[UpstreamSettingsKeys::apiKey] = upstreamSettings.apiKey;
        doc["lcd"] = lcd;
        doc["temps"] = temps;
        char tempFormatStr[2] = { tempControl.cc.tempFormat, '\0' };
        char modeStr[2] = { tempControl.cs.mode, '\0' };
        doc["temp_format"] = tempFormatStr;
        doc["mode"] = modeStr;

#ifdef HAS_BLUETOOTH
        tilt* grav_sensor = bt_scanner.get_tilt(extendedSettings.tiltGravSensor);
        if(grav_sensor != nullptr) {
            doc["gravity_raw"] = grav_sensor->getGravity();
        }
#endif

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, url, response, httpMethod::HTTP_PUT);

    // Check if we have any messages pending on the server, and set the flag if so
    {
        JsonDocument doc;
        deserializeJson(doc, response);

        if(doc["has_messages"].is<bool>() && doc["has_messages"].as<bool>()) {
            bool has_messages = doc["has_messages"].as<bool>();

            if(has_messages)
                messages_pending_on_server = true;
        }

        if(doc["updated_mode"].is<const char *>()) {
            char updated_mode = doc["updated_mode"].as<const char *>()[0];

            if (updated_mode == Modes::fridgeConstant || updated_mode == Modes::beerConstant || updated_mode == Modes::beerProfile ||
                updated_mode == Modes::off || updated_mode == Modes::test) {
                    // We have a new, valid mode. Update to it.
                    if(tempControl.cs.mode != updated_mode) {
                        Log.info("Updating to valid mode \"%c\" (0x%02X)\r\n", updated_mode, updated_mode);
                        tempControl.setMode(updated_mode);
                        send_status_ticker = true;  // Trigger a send to update the LCD
                    }
            } else {
                Log.error("Invalid mode \"%c\" (0x%02X)\r\n", updated_mode, updated_mode);
            }
        }

        if(doc["updated_setpoint"].is<const char *>()) {
            switch(tempControl.cs.mode) {
                case Modes::fridgeConstant:
                    SettingLoader::setFridgeSetting(doc["updated_setpoint"].as<const char *>());
                    send_status_ticker = true;  // Trigger a send to update the LCD
                    break;
                case Modes::beerConstant:
                case Modes::beerProfile:
                    SettingLoader::setBeerSetting(doc["updated_setpoint"].as<const char *>());
                    Log.info("Received updated setpoint \"%s\"\r\n", doc["updated_setpoint"].as<const char *>());
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
    std::string payload;
    char url[256] = "";
    std::string response;

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
    //     JsonDocument doc;

    //     doc[UpstreamSettingsKeys::deviceID] = upstreamSettings.deviceID;
    //     doc[UpstreamSettingsKeys::apiKey] = upstreamSettings.apiKey;

    //     // Serialize the JSON document
    //     serializeJson(doc, payload);
    // }

    send_json_str(payload, url, response, httpMethod::HTTP_GET);

    // Parse any messages that are on the server
    {
        JsonDocument doc;
        deserializeJson(doc, response);

        // Process any flags on the server
        if(doc[RestMessagesKeys::messages].is<JsonObject>()) {
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::updated_cs].is<bool>())
                messages.updated_cs = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_cs].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::updated_cc].is<bool>())
                messages.updated_cc = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_cc].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::updated_mt].is<bool>())
                messages.updated_mt = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_mt].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::updated_es].is<bool>())
                messages.updated_es = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_es].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::updated_devices].is<bool>())
                messages.updated_devices = doc[RestMessagesKeys::messages][RestMessagesKeys::updated_devices].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::default_cc].is<bool>())
                messages.default_cc = doc[RestMessagesKeys::messages][RestMessagesKeys::default_cc].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::default_cs].is<bool>())
                messages.default_cs = doc[RestMessagesKeys::messages][RestMessagesKeys::default_cs].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::reset_eeprom].is<bool>())
                messages.reset_eeprom = doc[RestMessagesKeys::messages][RestMessagesKeys::reset_eeprom].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::reset_connection].is<bool>())
                messages.reset_connection = doc[RestMessagesKeys::messages][RestMessagesKeys::reset_connection].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::restart_device].is<bool>())
                messages.restart_device = doc[RestMessagesKeys::messages][RestMessagesKeys::restart_device].as<bool>();
            if(doc[RestMessagesKeys::messages][RestMessagesKeys::refresh_config].is<bool>())
                messages.refresh_config = doc[RestMessagesKeys::messages][RestMessagesKeys::refresh_config].as<bool>();
        }
     
    }

    messages_pending_on_server = false;  // TODO - Remove once done with testing, as we should only unset this if we have a confirmed message count of 0
    return true;
}

bool restHandler::set_message_processed(const char* message_type_key) {
    std::string payload;
    char url[256] = "";
    std::string response;

    // We can't delete messages if we're not registered
    if(upstreamSettings.isRegistered() == false)
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::messages))
        return false;

    {
        JsonDocument doc;

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

