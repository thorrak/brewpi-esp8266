#ifdef ENABLE_HTTP_INTERFACE

#include <Arduino.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>

#include "ESPEepromAccess.h"  // Defines FILESYSTEM (and includes the approprite headers)

#include "uptime.h"
#include "resetreasons.h"
#include "http_server.h"
#include "TempControl.h"
// #include "sendData.h"
// #include "serialhandler.h"
#include "JsonMessages.h"
#include "DeviceManager.h"
#include "JsonKeys.h"
#include "rest/rest_send.h"

#include "extended_async_json_handler.h"


httpServer http_server;
AsyncWebServer asyncWebServer(WEB_SERVER_PORT);



// Settings Page Handlers

bool processUpstreamConfigUpdateJson(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;

    // Upstream Host
    if(json[UpstreamSettingsKeys::upstreamHost].is<const char*>()) {
        if (strlen(json[UpstreamSettingsKeys::upstreamHost]) <= 0) {
            // The user unset the upstream host - Clear it from memory
            upstreamSettings.upstreamHost[0] = '\0';
            upstreamSettings.deviceID[0] = '\0';  // Also clear the device ID
            Log.notice(F("Settings update, [upstreamHost]: unset.\r\n"));
        } else if (strlen(json[UpstreamSettingsKeys::upstreamHost]) >= 128 ) {
            Log.warning(F("Settings update error, [upstreamHost]:(%s) not valid.\r\n"), json[UpstreamSettingsKeys::upstreamHost].as<const char*>());
            failCount++;
        } else {
            // Valid - Update
            if(strcmp(json[UpstreamSettingsKeys::upstreamHost], upstreamSettings.upstreamHost) != 0) {
                strlcpy(upstreamSettings.upstreamHost, json[UpstreamSettingsKeys::upstreamHost].as<const char*>(), 128);
                upstreamSettings.deviceID[0] = '\0';  // Also clear the device ID
                Log.notice(F("Settings update, [upstreamHost]:(%s) applied.\r\n"), json[UpstreamSettingsKeys::upstreamHost].as<const char*>());
                saveSettings = true;
            }
        }
    }

    // Upstream Port
    if(json[UpstreamSettingsKeys::upstreamPort].is<uint16_t>()) {
        if((json[UpstreamSettingsKeys::upstreamPort] <= 0) || (json[UpstreamSettingsKeys::upstreamPort] > 65535)) {
            // This is actually impossible to reach, unless the port is 0.
            Log.warning("Invalid [upstreamPort]:(%u) received.\r\n", json[UpstreamSettingsKeys::upstreamPort].as<uint16_t>());
            failCount++;
        } else {
            //Valid - Update
            upstreamSettings.upstreamPort = json[UpstreamSettingsKeys::upstreamPort];
            upstreamSettings.deviceID[0] = '\0';  // Also clear the device ID
            Log.warning("Settings update, [upstreamPort]:(%d) applied.\r\n", json[UpstreamSettingsKeys::upstreamPort].as<uint16_t>());
            saveSettings = true;
        }
    } else {
        Log.warning("Invalid [upstreamPort]:(%s) received (wrong type).\r\n", json[UpstreamSettingsKeys::upstreamPort].as<const char*>());
        failCount++;
    }

    // Device ID
    // NOTE - We're not allowing the device ID to be changed via the API for now. Instead, it has to be reset with the upstream
    // if(json[UpstreamSettingsKeys::deviceID].is<const char*>()) {
    //     if (strlen(json[UpstreamSettingsKeys::deviceID]) <= 0) {
    //         // The user unset the upstream host - Clear it from memory
    //         upstreamSettings.deviceID[0] = '\0';
    //         Log.notice(F("Settings update, [deviceID]: unset.\r\n"));
    //     } else if (strlen(json[UpstreamSettingsKeys::deviceID]) >= 40 ) {
    //         Log.warning(F("Settings update error, [deviceID]:(%s) not valid.\r\n"), json[UpstreamSettingsKeys::deviceID].as<const char*>());
    //         failCount++;
    //     } else {
    //         if(strcmp(json[UpstreamSettingsKeys::deviceID], upstreamSettings.deviceID) != 0) {
    //             strlcpy(upstreamSettings.deviceID, json[UpstreamSettingsKeys::deviceID].as<const char*>(), 40);
    //             Log.notice(F("Settings update, [deviceID]:(%s) applied.\r\n"), json[UpstreamSettingsKeys::deviceID].as<const char*>());
    //             saveSettings = true;
    //         }
    //     }
    // }


    // Upstream Username
    if(json[UpstreamSettingsKeys::username].is<const char*>()) {
        if (strlen(json[UpstreamSettingsKeys::username]) <= 0) {
            // The user unset the upstream host - Clear it from memory
            upstreamSettings.username[0] = '\0';
            upstreamSettings.deviceID[0] = '\0';  // Also clear the device ID
            Log.notice(F("Settings update, [username]: unset.\r\n"));
        } else if (strlen(json[UpstreamSettingsKeys::username]) >= 128 ) {
            Log.warning(F("Settings update error, [username]:(%s) not valid.\r\n"), json[UpstreamSettingsKeys::username].as<const char*>());
            failCount++;
        } else {
            // Valid - Update
            if(strcmp(json[UpstreamSettingsKeys::username], upstreamSettings.username) != 0) {
                strlcpy(upstreamSettings.username, json[UpstreamSettingsKeys::username].as<const char*>(), 128);
                upstreamSettings.deviceID[0] = '\0';  // Also clear the device ID
                Log.notice(F("Settings update, [username]:(%s) applied.\r\n"), json[UpstreamSettingsKeys::username].as<const char*>());
                saveSettings = true;
            }
        }
    }

    // Upstream API Key
    // NOTE - Unused as of Jan 2024
    if(json[UpstreamSettingsKeys::apiKey].is<const char*>()) {
        if (strlen(json[UpstreamSettingsKeys::apiKey]) <= 0) {
            // The user unset the upstream host - Clear it from memory
            upstreamSettings.apiKey[0] = '\0';
            upstreamSettings.deviceID[0] = '\0';  // Also clear the device ID
            Log.notice(F("Settings update, [apiKey]: unset.\r\n"));
        } else if (strlen(json[UpstreamSettingsKeys::apiKey]) >= 40 ) {
            Log.warning(F("Settings update error, [apiKey]:(%s) not valid.\r\n"), json[UpstreamSettingsKeys::apiKey].as<const char*>());
            failCount++;
        } else {
            // Valid - Update
            if(strcmp(json[UpstreamSettingsKeys::apiKey], upstreamSettings.apiKey) != 0) {
                strlcpy(upstreamSettings.apiKey, json[UpstreamSettingsKeys::apiKey].as<const char*>(), sizeof(upstreamSettings.apiKey));
                upstreamSettings.deviceID[0] = '\0';  // Also clear the device ID
                Log.notice(F("Settings update, [apiKey]:(%s) applied.\r\n"), json[UpstreamSettingsKeys::apiKey].as<const char*>());
                saveSettings = true;
            }
        }
    }


    // Save
    if (failCount) {
        Log.error(F("Error: Invalid upstream configuration.\r\n"));
    } else {
        if(saveSettings == true) {
            upstreamSettings.storeToFilesystem();
        }
        upstreamSettings.upstreamRegistrationError = UpstreamSettings::upstreamRegErrorT::NOT_ATTEMPTED_REGISTRATION;
        rest_handler.register_device_ticker = true;
    }
    return failCount == 0;
}


bool processDeviceUpdateJson(const JsonDocument& json, bool triggerUpstreamUpdate) {
    DeviceDefinition dev;
    // Check for universally required keys
    if(!json[DeviceDefinitionKeys::chamber].is<uint8_t>() || !json[DeviceDefinitionKeys::beer].is<uint8_t>() || 
        !json[DeviceDefinitionKeys::function].is<uint8_t>() || !json[DeviceDefinitionKeys::hardware].is<uint8_t>()
    //    || !json[DeviceDefinitionKeys::deactivated].is<bool>()
    ) 
    {
        // We don't actually parse deactivated, so commenting out the check. If we add it later, we will need to check that we don't need to do
        // shenanigans like we do with invert below to handle all the various ways it can be sent to us.
        Log.warning(F("Invalid device definition received - missing required keys (c/f/h/b).\r\n"));
        return 1;
    }

    switch(json[DeviceDefinitionKeys::hardware].as<uint8_t>()) {
        case DEVICE_HARDWARE_PIN:

            if(!json[DeviceDefinitionKeys::pin].is<int>() || !(json[DeviceDefinitionKeys::invert].is<bool>() || json[DeviceDefinitionKeys::invert].is<const char *>() || json[DeviceDefinitionKeys::invert].is<uint8_t>())) {
                Log.warning(F("Invalid device definition received - missing required keys (p/x).\r\n"));
                return 1;
            }
            break;
        case DEVICE_HARDWARE_ONEWIRE_TEMP:
        case DEVICE_HARDWARE_BLUETOOTH_INKBIRD:
        case DEVICE_HARDWARE_BLUETOOTH_TILT:
            if(!json[DeviceDefinitionKeys::address].is<const char*>()) {
                Log.warning(F("Invalid device definition received - missing required keys (a).\r\n"));
                return 1;
            }
            break;
        case DEVICE_HARDWARE_TPLINK_SWITCH:
            if(!json[DeviceDefinitionKeys::address].is<const char*>() || !json[DeviceDefinitionKeys::child_id].is<const char*>()) {
                Log.warning(F("Invalid device definition received - missing required keys (a).\r\n"));
                return 1;
            }
            break;
        default:
            break;
    }
    http_server.dev = DeviceManager::readJsonIntoDeviceDef(json);                  // Parse the JSON into a DeviceDefinition object
    http_server.device_definition_update_requested = true;
    // dev = DeviceManager::readJsonIntoDeviceDef(json);                  // Parse the JSON into a DeviceDefinition object
    // TODO - Trigger upstream update
    return true;
}


// Allows us to process the device definition update in the main loop rather than in the async handler
void httpServer::processQueuedDeviceDefinition() {
    if(device_definition_update_requested) {
        DeviceConfig print = deviceManager.updateDeviceDefinition(dev);   // Save the device definition (if valid)
        device_definition_update_requested = false;
    }
}


bool processUpdateModeJson(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;

    // Temperature Control Mode
    if(json[ModeUpdateKeys::mode].is<const char *>()) {
        if (strlen(json[ModeUpdateKeys::mode]) == 1) {
            char new_mode = json[ModeUpdateKeys::mode].as<const char *>()[0];
            if (new_mode == Modes::fridgeConstant || new_mode == Modes::beerConstant || new_mode == Modes::beerProfile ||
                new_mode == Modes::off || new_mode == Modes::test) {
                // Mode is valid - Update
                if(new_mode != tempControl.getMode()) {
                    tempControl.setMode(new_mode);
                    Log.notice(F("Settings update, [newMode]:(%c) applied.\r\n"), new_mode);
                    saveSettings = true;
                } else {
                    Log.notice(F("Settings update, [newMode]:(%c) NOT applied - no change.\r\n"), new_mode);
                }
            } else {
                Log.warning(F("Settings update error, [newMode]:(%c) not valid.\r\n"), new_mode);
                failCount++;
            }
        } else {
            Log.warning(F("Settings update error, [newMode]:(%s) not a valid type.\r\n"), json[ModeUpdateKeys::mode].as<const char*>());
            failCount++;
        }
    }


    // Set Point
    if(json[ModeUpdateKeys::setpoint].is<double>()) {
        if(tempControl.getMode() != Modes::fridgeConstant && tempControl.getMode() != Modes::beerConstant && tempControl.getMode() != Modes::beerProfile) {
            Log.info(F("Settings update error, [setpoint]:(%s) current mode (%c) does not take a setpoint.\r\n"), json[ModeUpdateKeys::setpoint].as<const char*>(), tempControl.getMode());
        } else {
            char modeString[7];
            snprintf(modeString, 7, "%.1f", json[ModeUpdateKeys::setpoint].as<double>());

            temperature newTemp = stringToTemp(modeString);  // Also constrains the value to the valid range

            if(tempControl.getMode() == Modes::fridgeConstant) {
                tempControl.setFridgeTemp(newTemp);
                saveSettings = true;
            } else if(tempControl.getMode() == Modes::beerConstant || tempControl.getMode() == Modes::beerProfile) {
                tempControl.setBeerTemp(newTemp);
                saveSettings = true;
            } else {
                Log.error(F("Settings update error, [setpoint]:(%s) current mode (%c) does not take a setpoint (should never be reached).\r\n"), modeString, tempControl.getMode());
            }
        }
    }


    // Save
    if (failCount) {
        Log.error(F("Error: Invalid upstream configuration.\r\n"));
    } else {
        if(saveSettings == true) {
            // TODO - Force upstream cascade/send
            // upstreamSettings.storeToFilesystem();
        }
    }
    return failCount == 0;
}



bool processExtendedSettingsJson(const JsonDocument& json, bool triggerUpstreamUpdate) {
    uint8_t failCount = 0;
    bool saveSettings = false;
    bool saveMinTimes = false; 

    // Glycol Mode
    if(json[ExtendedSettingsKeys::glycol].is<bool>()) {
        if(extendedSettings.glycol != json[ExtendedSettingsKeys::glycol].as<bool>()) {
            extendedSettings.setGlycol(json[ExtendedSettingsKeys::glycol].as<bool>());
            saveSettings = true;
        }
    } else {
        Log.warning(F("Invalid [glycol]:(%s) received (wrong type).\r\n"), json[ExtendedSettingsKeys::glycol]);
        failCount++;
    }

    // Large TFT flag
    if(json[ExtendedSettingsKeys::largeTFT].is<bool>()) {
        if(extendedSettings.largeTFT != json[ExtendedSettingsKeys::largeTFT].as<bool>()) {
            extendedSettings.setLargeTFT(json[ExtendedSettingsKeys::largeTFT].as<bool>());
            saveSettings = true;
        }
    } else {
        Log.warning(F("Invalid [largeTFT]:(%s) received (wrong type).\r\n"), json[ExtendedSettingsKeys::largeTFT]);
        failCount++;
    }

    // Invert TFT Flag
    if(json[ExtendedSettingsKeys::invertTFT].is<bool>()) {
        if(extendedSettings.invertTFT != json[ExtendedSettingsKeys::invertTFT].as<bool>()) {
            extendedSettings.setInvertTFT(json[ExtendedSettingsKeys::invertTFT].as<bool>());
            saveSettings = true;
        }
    } else {
        Log.warning(F("Invalid [invertTFT]:(%s) received (wrong type).\r\n"), json[ExtendedSettingsKeys::invertTFT]);
        failCount++;
    }


#ifdef HAS_BLUETOOTH
    // Tilt Gravity Sensor
    if(json[ExtendedSettingsKeys::tiltGravSensor].is<std::string>()) {
        // Validate that it's valid
        if(extendedSettings.tiltGravSensor != NimBLEAddress(json[ExtendedSettingsKeys::tiltGravSensor].as<std::string>(), 1)) {
            // Tilts use address type 1 ("random", which (correctly!) indicates they didn't buy a MAC block)
            extendedSettings.setTiltGravSensor(NimBLEAddress(json[ExtendedSettingsKeys::tiltGravSensor].as<std::string>(), 1));
            saveSettings = true;
        }
    }
#endif

    // SETTINGS_CHOICE
    if(json[MinTimesKeys::SETTINGS_CHOICE].is<uint8_t>()) {
        // Validate that it's valid and different
        if(minTimes.settings_choice != json[MinTimesKeys::SETTINGS_CHOICE].as<uint8_t>() && json[MinTimesKeys::SETTINGS_CHOICE].as<uint8_t>() <= MIN_TIMES_CUSTOM) {
            minTimes.settings_choice = (MinTimesSettingsChoice) json[MinTimesKeys::SETTINGS_CHOICE].as<uint8_t>();
            saveMinTimes = true;
        }
    }


    if(minTimes.settings_choice == MIN_TIMES_CUSTOM) {
        // We only care about the other keys if we're in custom mode -- otherwise the call to setDefaults below will overwrite them

        // MIN_COOL_OFF_TIME
        if(json[MinTimesKeys::MIN_COOL_OFF_TIME].is<uint16_t>()) {
            if(minTimes.MIN_COOL_OFF_TIME != json[MinTimesKeys::MIN_COOL_OFF_TIME].as<uint16_t>()) {
                minTimes.MIN_COOL_OFF_TIME = json[MinTimesKeys::MIN_COOL_OFF_TIME].as<uint16_t>();
                saveMinTimes = true;
            }
        }


        // MIN_HEAT_OFF_TIME
        if(json[MinTimesKeys::MIN_HEAT_OFF_TIME].is<uint16_t>()) {
            if(minTimes.MIN_HEAT_OFF_TIME != json[MinTimesKeys::MIN_HEAT_OFF_TIME].as<uint16_t>()) {
                minTimes.MIN_HEAT_OFF_TIME = json[MinTimesKeys::MIN_HEAT_OFF_TIME].as<uint16_t>();
                saveMinTimes = true;
            }
        }

        // MIN_COOL_ON_TIME
        if(json[MinTimesKeys::MIN_COOL_ON_TIME].is<uint16_t>()) {
            if(minTimes.MIN_COOL_ON_TIME != json[MinTimesKeys::MIN_COOL_ON_TIME].as<uint16_t>()) {
                minTimes.MIN_COOL_ON_TIME = json[MinTimesKeys::MIN_COOL_ON_TIME].as<uint16_t>();
                saveMinTimes = true;
            }
        }

        // MIN_HEAT_ON_TIME
        if(json[MinTimesKeys::MIN_HEAT_ON_TIME].is<uint16_t>()) {
            if(minTimes.MIN_HEAT_ON_TIME != json[MinTimesKeys::MIN_HEAT_ON_TIME].as<uint16_t>()) {
                minTimes.MIN_HEAT_ON_TIME = json[MinTimesKeys::MIN_HEAT_ON_TIME].as<uint16_t>();
                saveMinTimes = true;
            }
        }


        // MIN_COOL_OFF_TIME_FRIDGE_CONSTANT
        if(json[MinTimesKeys::MIN_COOL_OFF_TIME_FRIDGE_CONSTANT].is<uint16_t>()) {
            if(minTimes.MIN_COOL_OFF_TIME_FRIDGE_CONSTANT != json[MinTimesKeys::MIN_COOL_OFF_TIME_FRIDGE_CONSTANT].as<uint16_t>()) {
                minTimes.MIN_COOL_OFF_TIME_FRIDGE_CONSTANT = json[MinTimesKeys::MIN_COOL_OFF_TIME_FRIDGE_CONSTANT].as<uint16_t>();
                saveMinTimes = true;
            }
        }

        // MIN_SWITCH_TIME
        if(json[MinTimesKeys::MIN_SWITCH_TIME].is<uint16_t>()) {
            if(minTimes.MIN_SWITCH_TIME != json[MinTimesKeys::MIN_SWITCH_TIME].as<uint16_t>()) {
                minTimes.MIN_SWITCH_TIME = json[MinTimesKeys::MIN_SWITCH_TIME].as<uint16_t>();
                saveMinTimes = true;
            }
        }

        // COOL_PEAK_DETECT_TIME
        if(json[MinTimesKeys::COOL_PEAK_DETECT_TIME].is<uint16_t>()) {
            if(minTimes.COOL_PEAK_DETECT_TIME != json[MinTimesKeys::COOL_PEAK_DETECT_TIME].as<uint16_t>()) {
                minTimes.COOL_PEAK_DETECT_TIME = json[MinTimesKeys::COOL_PEAK_DETECT_TIME].as<uint16_t>();
                saveMinTimes = true;
            }
        }


        // HEAT_PEAK_DETECT_TIME
        if(json[MinTimesKeys::HEAT_PEAK_DETECT_TIME].is<uint16_t>()) {
            if(minTimes.HEAT_PEAK_DETECT_TIME != json[MinTimesKeys::HEAT_PEAK_DETECT_TIME].as<uint16_t>()) {
                minTimes.HEAT_PEAK_DETECT_TIME = json[MinTimesKeys::HEAT_PEAK_DETECT_TIME].as<uint16_t>();
                saveMinTimes = true;
            }
        }
    }

    // Save
    if (failCount) {
        Log.error(F("Error: Invalid extended settings configuration.\r\n"));
    } else {
        if(saveSettings == true) {
            extendedSettings.storeToFilesystem();
            // TODO - Force upstream cascade/send
        }
        if(saveMinTimes == true) {
            minTimes.setDefaults(); // This will set defaults if defaults/lowdelay mode is set -- otherwise its a noop for custom mode
            minTimes.storeToFilesystem();
            // TODO - Force upstream cascade/send
        }
    }
    return failCount == 0;
}


// bool processActionJson(const JsonDocument& json) {

//     if(!json["action"].is<const char*>()) {
//         Log.warning(F("Action error - Action key is not a string.\r\n"));
//         return false;
//     }

//     if(strcmp(json["action"], "reset_connection") == 0) {
//         Log.notice(F("Action [reset_connection] received\r\n"));
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


// There may be a way to combine the following using virtual functions, but I'm not going to worry about that for now
void serveExtendedSettings(JsonDocument &doc) {
    JsonDocument extended_settings;
    JsonDocument min_times;

    extendedSettings.toJson(extended_settings);
    minTimes.toJson(min_times);

    doc["extendedSettings"] = extended_settings;
    doc["minTimes"] = min_times;
}

void serveUpstreamSettings(JsonDocument &doc) {
    upstreamSettings.toJson(doc);
}


// About Page Handlers
void uptime(JsonDocument &doc) {
    Log.verbose(F("Serving uptime.\r\n"));

    doc["days"] = uptimeDays();
    doc["hours"] = uptimeHours();
    doc["minutes"] = uptimeMinutes();;
    doc["seconds"] = uptimeSeconds();
    doc["millis"] = uptimeMillis();

}


void heap(JsonDocument &doc) {
    Log.verbose(F("Serving heap information.\r\n"));

    const uint32_t free = ESP.getFreeHeap();
#ifdef ESP32
    const uint32_t max = ESP.getMaxAllocHeap();
#elif defined(ESP8266)
    const uint32_t max = ESP.getMaxFreeBlockSize();
#endif
    const uint8_t frag = 100 - (max * 100) / free;

    doc["free"] = free;
    doc["max"] = max;
    doc["frag"] = frag;
}


void reset_reason(JsonDocument &doc) {
    Log.verbose(F("Serving reset reason.\r\n"));

#ifdef ESP32
    const int reset = (int)esp_reset_reason();
    doc["reason"] = resetReason[reset];
    doc["description"] = resetDescription[reset];
#elif defined(ESP8266)
    doc["reason"] = ESP.getResetReason();
    doc["description"] = "N/A";
#endif

}


// Static page routing
void httpServer::setStaticPages() {
    // Define the base static page handlers
    asyncWebServer.serveStatic("/", FILESYSTEM, "/index.html").setCacheControl("max-age=600");
    asyncWebServer.serveStatic("/index.html", FILESYSTEM, "/index.html").setCacheControl("max-age=600");

    // Define Vue routes
    const char* vueRoutes[] = {
        "/upstream", 
        "/devices",
        "/about", 
        "/settings"
    };


    // Serve static pages for Vue routes and their trailing-slash versions
    for (const char* route : vueRoutes) {
        asyncWebServer.serveStatic(route, FILESYSTEM, "/index.html").setCacheControl("max-age=600");

        // Serve the same route with a trailing slash
        String routeWithSlash = String(route) + "/";
        asyncWebServer.serveStatic(routeWithSlash.c_str(), FILESYSTEM, "/index.html").setCacheControl("max-age=600");
    }

    // Legacy static page handlers
    // TODO - Determine if this can be deleted
    asyncWebServer.serveStatic("/404/", FILESYSTEM, "/404.html").setCacheControl("max-age=600");

}


void httpServer::setJsonPages() {
    struct Endpoint {
        const char* path;
        void (*handler)(JsonDocument&);
    };

    const Endpoint endpoints[] = {
        {"/api/version/", versionInfoJson},
        {"/api/lcd/", getLcdContentJson},
        {"/api/temps/", printTemperaturesJson},
        {"/api/cs/", tempControl.getControlSettingsDoc},
        {"/api/cc/", tempControl.getControlConstantsDoc},
        {"/api/cv/", tempControl.getControlVariablesDoc},
        {"/api/all_temp_control/", getFullTemperatureControlJson},
        {"/api/devices/", DeviceManager::enumerateHardware},
        {"/api/extended/", serveExtendedSettings},
        {"/api/upstream/", serveUpstreamSettings},
        {"/api/uptime/", uptime},
        {"/api/heap/", heap},
        {"/api/resetreason/", reset_reason},
    };

    for (const auto& endpoint : endpoints) {
        asyncWebServer.addHandler(new GetAsyncCallbackJsonWebHandler(endpoint.path, endpoint.handler));
    }
}


void httpServer::setPutPages() {
    struct Endpoint {
        const char* path;
        bool (*handler)(const JsonDocument&, bool);
    };

    const Endpoint endpoints[] = {
        {"/api/upstream/", processUpstreamConfigUpdateJson},
        {"/api/devices/", processDeviceUpdateJson},
        {"/api/mode/", processUpdateModeJson},
        {"/api/extended/", processExtendedSettingsJson},
    };

    for (const auto& endpoint : endpoints) {
        asyncWebServer.addHandler(new PutAsyncCallbackJsonWebHandler(endpoint.path, endpoint.handler));
    }
}


void httpServer::init() {
    setStaticPages();
    setJsonPages();
    setPutPages();

    // File not found handler
    asyncWebServer.onNotFound([](AsyncWebServerRequest *request) {
        if (!http_server.handleFileRead(request, request->url())) {
            request->send(404, "text/plain", "Not Found");
        }
    });

    // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    asyncWebServer.begin();
    Log.notice(F("HTTP server started. Open: http://%s.local/ to view application.\r\n"), WiFi.getHostname());
}



#endif
