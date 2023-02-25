#ifdef ENABLE_HTTP_INTERFACE

#include <Arduino.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>
// #define LCBURL_MDNS
// #include <LCBUrl.h>
// #include <WiFi.h>
// #include <WiFiClient.h>

#include "ESPEepromAccess.h"  // Defines FILESYSTEM (and includes the approprite headers)

#include "uptime.h"
// #include "version.h"
// #include "wifi_setup.h"

// #include "jsonconfig.h"

#include "resetreasons.h"
#include "http_server.h"
#include "TempControl.h"
// #include "sendData.h"
// #include "serialhandler.h"
#include "JsonMessages.h"
#include "DeviceManager.h"
#include "JsonKeys.h"


httpServer http_server;



// Settings Page Handlers

uint8_t processUpstreamConfigUpdateJson(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;

    // Upstream Host
    if(json.containsKey(UpstreamSettingsKeys::upstreamHost)) {
        if (strlen(json[UpstreamSettingsKeys::upstreamHost]) <= 0) {
            // The user unset the upstream host - Clear it from memory
            upstreamSettings.upstreamHost[0] = '\0';
            Log.notice(F("Settings update, [upstreamHost]: unset.\r\n"));
        } else if (strlen(json[UpstreamSettingsKeys::upstreamHost]) >= 128 ) {
            Log.warning(F("Settings update error, [upstreamHost]:(%s) not valid.\r\n"), json[UpstreamSettingsKeys::upstreamHost].as<const char*>());
            failCount++;
        } else {
            // Valid - Update
            if(strcmp(json[UpstreamSettingsKeys::upstreamHost], upstreamSettings.upstreamHost) != 0) {
                strlcpy(upstreamSettings.upstreamHost, json[UpstreamSettingsKeys::upstreamHost].as<const char*>(), 128);
                Log.notice(F("Settings update, [upstreamHost]:(%s) applied.\r\n"), json[UpstreamSettingsKeys::upstreamHost].as<const char*>());
                saveSettings = true;
            }
        }
    }

    // Upstream Port
    if(json.containsKey(UpstreamSettingsKeys::upstreamPort)) {
        if(json[UpstreamSettingsKeys::upstreamPort].is<uint16_t>()) {
            if((json[UpstreamSettingsKeys::upstreamPort] < 0) || (json[UpstreamSettingsKeys::upstreamPort] > 65535)) {
                Log.warning(F("Invalid [upstreamPort]:(%u) received.\r\n"), json[UpstreamSettingsKeys::upstreamPort]);
                failCount++;
            } else {
                //Valid - Update
                upstreamSettings.upstreamPort = json[UpstreamSettingsKeys::upstreamPort];
                Log.notice(F("Settings update, [upstreamPort]:(%d) applied.\r\n"), json[UpstreamSettingsKeys::upstreamHost].as<uint16_t>());
                saveSettings = true;
            }
        } else {
            Log.warning(F("Invalid [upstreamPort]:(%s) received (wrong type).\r\n"), json[UpstreamSettingsKeys::upstreamPort]);
            failCount++;
        }
    }

    // Device ID
    // NOTE - We're not allowing the device ID to be changed via the API for now. Instead, it has to be reset with the upstream
    // if(json.containsKey(UpstreamSettingsKeys::deviceID)) {
    //     if (strlen(json[UpstreamSettingsKeys::deviceID]) <= 0) {
    //         // The user unset the upstream host - Clear it from memory
    //         upstreamSettings.deviceID[0] = '\0';
    //         Log.notice(F("Settings update, [deviceID]: unset.\r\n"));
    //     } else if (strlen(json[UpstreamSettingsKeys::deviceID]) >= 128 ) {
    //         Log.warning(F("Settings update error, [deviceID]:(%s) not valid.\r\n"), json[UpstreamSettingsKeys::deviceID].as<const char*>());
    //         failCount++;
    //     } else {
    //         if(strcmp(json[UpstreamSettingsKeys::deviceID], upstreamSettings.deviceID) != 0) {
    //             strlcpy(upstreamSettings.deviceID, json[UpstreamSettingsKeys::deviceID].as<const char*>(), 64);
    //             Log.notice(F("Settings update, [deviceID]:(%s) applied.\r\n"), json[UpstreamSettingsKeys::deviceID].as<const char*>());
    //             saveSettings = true;
    //         }
    //     }
    // }


    // Device ID Reset
    if(json.containsKey("resetDeviceID")) {
        if(json["resetDeviceID"].is<bool>()) {
            if(json["resetDeviceID"]) {
                // The user wants to reset the device ID
                Log.notice(F("Settings update, [resetDeviceID]: true.\r\n"));
                upstreamSettings.deviceID[0] = '\0';
                saveSettings = true;
            }
        } else {
            Log.warning(F("Invalid [resetDeviceID]:(%s) received (wrong type).\r\n"), json["resetDeviceID"]);
            failCount++;
        }
    }


    // //////  Generic Settings
    // // mDNS ID
    // if(json.containsKey("mdnsID")) {
    //     // Set hostname
    //     LCBUrl url;
    //     if (!url.isValidLabel(json["mdnsID"])) {
    //         Log.warning(F("Settings update error, [mdnsID]:(%s) not valid.\r\n"), json["mdnsID"]);
    //         failCount++;
    //     } else {
    //         if (strcmp(config.mdnsID, json["mdnsID"].as<const char*>()) != 0) {
    //             hostnamechanged = true;
    //             strlcpy(config.mdnsID, json["mdnsID"].as<const char*>(), 32);
    //             Log.notice(F("Settings update, [mdnsID]:(%s) applied.\r\n"), json["mdnsID"].as<const char*>());
    //         } else {
    //             Log.notice(F("Settings update, [mdnsID]:(%s) NOT applied - no change.\r\n"), json["mdnsID"].as<const char*>());
    //         }

    //     }
    // }


    // Save
    if (failCount) {
        Log.error(F("Error: Invalid upstream configuration.\r\n"));
    } else {
        if(saveSettings == true) {
            // TODO - Force upstream cascade/send
            upstreamSettings.storeToSpiffs();
        }
    }
    return failCount;
}


uint8_t processDeviceUpdateJson(const DynamicJsonDocument& json, bool triggerUpstreamUpdate) {
    DeviceDefinition dev;

    // Check for universally required keys
    if(!json.containsKey(DeviceDefinitionKeys::chamber) || !json.containsKey(DeviceDefinitionKeys::beer) || 
       !json.containsKey(DeviceDefinitionKeys::function) || !json.containsKey(DeviceDefinitionKeys::hardware) ||
       !json.containsKey(DeviceDefinitionKeys::pin) || !json.containsKey(DeviceDefinitionKeys::invert) ||
       !json.containsKey(DeviceDefinitionKeys::deactivated)) {

        Log.warning(F("Invalid device definition received - missing required keys.\r\n"));
        return 1;
    }


	  switch(json[DeviceDefinitionKeys::hardware].as<uint8_t>()) {
		case DEVICE_HARDWARE_ONEWIRE_TEMP:
		case DEVICE_HARDWARE_BLUETOOTH_INKBIRD:
		case DEVICE_HARDWARE_BLUETOOTH_TILT:
            if(!json.containsKey(DeviceDefinitionKeys::address)) {
                Log.warning(F("Invalid device definition received - missing required keys.\r\n"));
                return 1;
            }
			break;
		case DEVICE_HARDWARE_TPLINK_SWITCH:
            if(!json.containsKey(DeviceDefinitionKeys::address) || !json.containsKey(DeviceDefinitionKeys::child_id)) {
                Log.warning(F("Invalid device definition received - missing required keys.\r\n"));
                return 1;
            }
			break;
		default:
			break;
	  }

    dev = DeviceManager::readJsonIntoDeviceDef(json);                  // Parse the JSON into a DeviceDefinition object
    DeviceConfig print = deviceManager.updateDeviceDefinition(dev);   // Save the device definition (if valid)
    // TODO - Trigger upstream update
    return 0;
}


// uint8_t processSettingsUpdateJson(const JsonDocument& json) {
//     uint8_t failCount = 0;
//     bool hostnamechanged = false;


//     //////  Generic Settings
//     // mDNS ID
//     if(json.containsKey("mdnsID")) {
//         // Set hostname
//         LCBUrl url;
//         if (!url.isValidLabel(json["mdnsID"])) {
//             Log.warning(F("Settings update error, [mdnsID]:(%s) not valid.\r\n"), json["mdnsID"]);
//             failCount++;
//         } else {
//             if (strcmp(config.mdnsID, json["mdnsID"].as<const char*>()) != 0) {
//                 hostnamechanged = true;
//                 strlcpy(config.mdnsID, json["mdnsID"].as<const char*>(), 32);
//                 Log.notice(F("Settings update, [mdnsID]:(%s) applied.\r\n"), json["mdnsID"].as<const char*>());
//             } else {
//                 Log.notice(F("Settings update, [mdnsID]:(%s) NOT applied - no change.\r\n"), json["mdnsID"].as<const char*>());
//             }

//         }
//     }


//     // invertTFT
//     if(json.containsKey("invertTFT")) {
//         if(json["invertTFT"].is<bool>()) {
//             if(config.invertTFT != json["invertTFT"].as<bool>())
//                 http_server.lcd_reinit_rqd = true;
//             config.invertTFT = json["invertTFT"];
//             if(json["invertTFT"].as<bool>())
//                 Log.notice(F("Settings update, [invertTFT]:(True) applied.\r\n"));
//             else
//                 Log.notice(F("Settings update, [invertTFT]:(False) applied.\r\n"));
//         } else {
//             Log.warning(F("Settings update error, [invertTFT]:(%s) not valid.\r\n"), json["invertTFT"].as<const char*>());
//             failCount++;
//         }
//     }


//     // Process everything we were passed
//     if (failCount) {
//         Log.error(F("Error: Invalid controller configuration.\r\n"));
//     } else {
//         if (config.save()) {
//             if (hostnamechanged) {
//                 // We reset hostname, process
//                 hostnamechanged = false;
//                 http_server.name_reset_requested = true;
//                 Log.notice(F("Received new mDNSid, queued network reset.\r\n"));
//             }
//         } else {
//             Log.error(F("Error: Unable to save controller configuration data.\r\n"));
//             failCount++;
//         }
//     }
//     return failCount;

// }


// bool processActionJson(const JsonDocument& json) {

//     if(!json.containsKey("action")) {
//         Log.warning(F("Action error - No action key found in json.\r\n"));
//         return false;
//     }

//     if(!json["action"].is<const char*>()) {
//         Log.warning(F("Action error - Action key is not a string.\r\n"));
//         return false;
//     }

//     if(strcmp(json["action"], "reset_wifi") == 0) {
//         Log.notice(F("Action [reset_wifi] received\r\n"));
//         http_server.wifi_reset_requested = true;
//         http_server.restart_requested = true;  // A restart is implicit in wifi_reset_requested, but explicitly specifying here anyways
//     }

//     if(strcmp(json["action"], "restart") == 0) {
//         Log.notice(F("Action [restart] received\r\n"));
//         http_server.restart_requested = true;
//     }

//     if(strcmp(json["action"], "reset_config") == 0) {
//         Log.notice(F("Action [reset_config] received\r\n"));
//         http_server.config_reset_requested = true;
//         http_server.restart_requested = true;  // A restart is generally triggered when setting config_reset_requested, but explicitly specifying here anyways
//     }

//     if(strcmp(json["action"], "reset_all") == 0) {
//         Log.notice(F("Action [reset_all] received\r\n"));
//         http_server.config_reset_requested = true;
//         http_server.wifi_reset_requested = true;
//         http_server.restart_requested = true;
//     }

// #ifndef DISABLE_OTA_UPDATES
//     if(strcmp(json["action"], "ota") == 0) {
//         Log.notice(F("Action [ota] received\r\n"));
//         http_server.ota_update_requested = true;
//     }
// #endif

//     return true;
// }



//-----------------------------------------------------------------------------------------

void httpServer::genericServeJson(void(*jsonFunc)(DynamicJsonDocument&)) {
    String serializedJson;  // Use String here to prevent stack overflow
    DynamicJsonDocument doc(8192);
    jsonFunc(doc);
    serializeJson(doc, serializedJson);
    doc.clear();
    web_server->send(200, "application/json", serializedJson);
}

// There may be a way to combine the following using virtual functions, but I'm not going to worry about that for now
void httpServer::serveExtendedSettings() {
    DynamicJsonDocument doc(2048);
    extendedSettings.toJson(doc);
    char serializedJson[2048];
    serializeJson(doc, serializedJson);
    doc.clear();
    web_server->send(200, "application/json", serializedJson);
}

void httpServer::serveUpstreamSettings() {
    DynamicJsonDocument doc(1024);
    upstreamSettings.toJson(doc);
    char serializedJson[2048];
    serializeJson(doc, serializedJson);
    doc.clear();
    web_server->send(200, "application/json", serializedJson);
}




// // About Page Handlers
// //
// void version_info(AsyncWebServerRequest *request) {
//     Log.verbose(F("Serving version.\r\n"));
//     StaticJsonDocument<96> doc;

//     doc["version"] = version();
//     doc["ota_status"] = ota_status;
//     // doc["branch"] = branch();
//     // doc["build"] = build();

//     if(millis() > (ota_last_check_at + OTA_CHECK_PERIOD) && ota_status == no_upgrade)
//         ota_status = pending_check;

//     char output[96];
//     serializeJson(doc, output);

//     web_server->send(200, "application/json", output);
// }


void httpServer::uptime() {
    Log.verbose(F("Serving uptime.\r\n"));
    StaticJsonDocument<96> doc;

    doc["days"] = uptimeDays();
    doc["hours"] = uptimeHours();
    doc["minutes"] = uptimeMinutes();;
    doc["seconds"] = uptimeSeconds();
    doc["millis"] = uptimeMillis();

    char output[96];
    serializeJson(doc, output);

    web_server->send(200, "application/json", output);
}


void httpServer::heap() {
    Log.verbose(F("Serving heap information.\r\n"));
    StaticJsonDocument<48> doc;

    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;

    doc["free"] = free;
    doc["max"] = max;
    doc["frag"] = frag;

    char output[48];
    serializeJson(doc, output);

    web_server->send(200, "application/json", output);
}


void httpServer::reset_reason() {
    Log.verbose(F("Serving reset reason.\r\n"));
    StaticJsonDocument<128> doc;

    const int reset = (int)esp_reset_reason();

    doc["reason"] = resetReason[reset];
    doc["description"] = resetDescription[reset];

    char output[128];
    serializeJson(doc, output);

    web_server->send(200, "application/json", output);
}


void httpServer::setStaticPages() {
    // Static page handlers - Vue
    web_server->serveStatic("/", FILESYSTEM, "/index.html", "max-age=600");
    web_server->serveStatic("/index.html", FILESYSTEM, "/index.html", "max-age=600");

    // Vue routes
    web_server->serveStatic("/upstream/", FILESYSTEM, "/index.html", "max-age=600");
    web_server->serveStatic("/devices/", FILESYSTEM, "/index.html", "max-age=600");
    web_server->serveStatic("/about/", FILESYSTEM, "/index.html", "max-age=600");

    // Legacy static page handlers
    web_server->serveStatic("/404/", FILESYSTEM, "/404.html", "max-age=600");
}


void httpServer::setJsonPages() {
    // Controller Version Stats
    web_server->on("/api/version/", HTTP_GET, [&]() {
        genericServeJson(&versionInfoJson);
    });

    // Controller Legacy LCD Output
    web_server->on("/api/lcd/", HTTP_GET, [&]() {
        genericServeJson(&getLcdContentJson);
    });

    // Controller Temps (logging format)
    web_server->on("/api/temps/", HTTP_GET, [&]() {
        genericServeJson(&printTemperaturesJson);
    });

    // Temp Control Settings (cs)
    web_server->on("/api/cs/", HTTP_GET, [&]() {
        genericServeJson(&tempControl.getControlSettingsDoc);
    });

    // Temp Control Constants (cc)
    web_server->on("/api/cc/", HTTP_GET, [&]() {
        genericServeJson(&tempControl.getControlConstantsDoc);
    });

    // Temp Control Variables (cv)
    web_server->on("/api/cv/", HTTP_GET, [&]() {
        genericServeJson(&tempControl.getControlVariablesDoc);
    });

    // Full Temp Control Settings (cs, cc, cv, logging format)
    web_server->on("/api/all_temp_control/", HTTP_GET, [&]() {
        genericServeJson(&getFullTemperatureControlJson);
    });

    // Full Temp Control Settings (cs, cc, cv, logging format)
    web_server->on("/api/devices/", HTTP_GET, [&]() {
        genericServeJson(&DeviceManager::enumerateHardware);
    });

    // Extended (non-stock-brewpi) Settings
    web_server->on("/api/extended/", HTTP_GET, [&]() {
        serveExtendedSettings();
    });

    // "Upstream" (Fermentrack REST) Settings
    web_server->on("/api/upstream/", HTTP_GET, [&]() {
        serveUpstreamSettings();
    });


    web_server->on("/api/uptime/", HTTP_GET, [&]() {
        uptime();
    });
    web_server->on("/api/heap/", HTTP_GET, [&]() {
        heap();
    });
    web_server->on("/api/resetreason/", HTTP_GET, [&]() {
        reset_reason();
    });
}


void httpServer::processJsonRequest(const char* uri, uint8_t (*handler)(const DynamicJsonDocument& json, bool triggerUpstreamUpdate)) {
    // Handler for configuration options
    char message[200] = "";
    uint8_t errors = 0;
    uint16_t status_code = 200;
    StaticJsonDocument<200> response;
    Log.verbose(F("Processing %s\r\n"), uri);

    DynamicJsonDocument json(8096);
    DeserializationError error = deserializeJson(json, web_server->arg("plain"));
    if (error) {
        Log.error(F("Error parsing JSON: %s\r\n"), error.c_str());
        response["message"] = "Unable to parse JSON";
        status_code = 400;
    } else {
        errors = handler(json, true);  // Apply the handler to the data (and trigger an upstream update)

        if(errors == 0) {
            response["message"] = "Update processed successfully";
        } else {
            response["message"] = "Unable to process update";
            status_code = 400;
        }    
    }

    serializeJson(response, message);
    web_server->send(status_code, "application/json", message);
    
}


void httpServer::setJsonHandlers() {

    web_server->on("/api/upstream/", HTTP_PUT, [&]() {
        processJsonRequest("/api/upstream/", &processUpstreamConfigUpdateJson);
    });

    web_server->on("/api/devices/", HTTP_PUT, [&]() {
        processJsonRequest("/api/devices/", &processDeviceUpdateJson);
    });

    // AsyncCallbackJsonWebHandler* processAction = new AsyncCallbackJsonWebHandler("/api/action/", [](AsyncWebServerRequest *request, JsonVariant const &json) {
    //     // TODO - Adapt this to use the new processJsonRequest function
    //     // Handler for configuration options
    //     char message[200] = "";
    //     StaticJsonDocument<200> response;
    //     Log.verbose(F("Processing /api/action/\r\n"));

    //     StaticJsonDocument<200> data;
    //     if (json.is<JsonArray>())
    //     {
    //         data = json.as<JsonArray>();
    //     }
    //     else if (json.is<JsonObject>())
    //     {
    //         data = json.as<JsonObject>();
    //     }

    //     // char serialized_mesasage[400];
    //     // Log.verbose(F("Received message: \r\n"));
    //     // serializeJson(data, serialized_mesasage);
    //     // Log.verbose(serialized_mesasage);

    //     if(processActionJson(data)) {
    //         response["message"] = "Update processed successfully";
    //     } else {
    //         response["message"] = "Unable to process update";
    //     }
        
    //     serializeJson(response, message);
    //     web_server->send(200, "application/json", message);
    // });
    // web_server->addHandler(processAction);

}


void httpServer::init() {
    web_server = new WebServer(WEB_SERVER_PORT);

    setStaticPages();
    setJsonPages();
    setJsonHandlers();

    // File not found handler
    web_server->onNotFound([&]() {
        String pathWithGz = web_server->uri() + ".gz";
        if (web_server->method() == HTTP_OPTIONS) {
            web_server->send(200);
        } else if(exists(web_server->uri()) || exists(pathWithGz)) {
            // WebServer doesn't automatically serve files, so we need to do that here unless we want to
            // manually add every single file to setStaticPages(). 
            handleFileRead(web_server->uri());
        } else {
            Log.verbose(F("Serving 404 for request to %s.\r\n"), web_server->uri().c_str());
            redirect("/404/");
        }
    });

    // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    web_server->begin();
    Log.notice(F("HTTP server started. Open: http://%s.local/ to view application.\r\n"), WiFi.getHostname());
}



#endif
