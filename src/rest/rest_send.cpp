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


restHandler rest_handler; // Global data sender


restHandler::restHandler() {
    bool send_full_config_ticker = false;
    bool send_lcd_ticker = false;
}

void restHandler::init()
{
    // Set up timers
    fullConfigTicker.once(45, [](){rest_handler.send_full_config_ticker=true;});
    lcdTicker.once(35, [](){rest_handler.send_lcd_ticker = true;});
}


void restHandler::get_useragent(char *ua, size_t size) {
    snprintf(ua, size,
        "brewpi-esp/%s (commit %s)",
        FIRMWARE_REVISION,
        Config::Version::git_tag
    );
}


// bool restHandler::send_data() {
//     send_to_taplistio(*this);
//     send_to_mqtt(*this);
//     return true;
// }



sendResult restHandler::send_json_str(String &payload, const char *url) {
    char auth_header[64];
    char userAgent[128];
    int httpResponseCode;
    sendResult result;

    send_lock = true;

    if (WiFiClass::status() != WL_CONNECTED) {
        Serial.print(F("send_json_str: Wifi not connected, skipping send.\r\n"));
        // Log.verbose(F("send_json_str: Wifi not connected, skipping send.\r\n"));
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
    WiFiClient client;
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
                httpResponseCode = http.PUT(payload);  // TODO - Technically should be either PUT, POST, or PATCH. Have to figure that out later. 

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

    send_json_str(payload, "http://www.fermentrack.com/api/bluetooth_crash/");
    return true;
    
}


void restHandler::get_url(char *url, size_t size, const char *path) {
    if(upstreamSettings.upstreamPort == 80) {
        snprintf(url, size, "http://%s%s", upstreamSettings.upstreamHost, path);
    } else {
        snprintf(url, size, "http://%s:%d%s", upstreamSettings.upstreamHost, upstreamSettings.upstreamPort, path);
    }
}


bool restHandler::send_full_config() {
    const char *url;
    String payload;

    // Only send if the semaphore is set - otherwise return
    if(!send_full_config_ticker)
        return false;
    else
        send_full_config_ticker = false;

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

        tempControl.getControlSettingsDoc(cs);
        tempControl.getControlConstantsDoc(cc);
        tempControl.getControlVariablesDoc(cv);

        EnumerateHardware spec;
        spec.values = 0;  // Change if we want to poll values here as well
        deviceManager.enumerateHardware(devices, spec);


        char guid[20];

        getGuid(guid);


        doc["cs"] = cs.as<JsonObject>();
        doc["cc"] = cc.as<JsonObject>();
        doc["cv"] = cv.as<JsonObject>();
        doc["devices"] = devices.as<JsonObject>();

        doc["uptime"] = esp_timer_get_time();
        doc["device_id"] = guid;

        // Serialize the JSON document
        serializeJson(doc, payload);
    }

    send_json_str(payload, "http://www.fermentrack.com/api/bluetooth_crash/");
    return true;
    
}


