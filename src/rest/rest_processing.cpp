#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <ArduinoJson.h>
#include <string>
#include <ctime>

#include <thorlog.h>
#include <thorlog_espidf.h>
#include <esp_system.h>

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
#include "SettingsManager.h"
#include "SettingLoader.h"


void restHandler::apply_pending_messages() {
    // Restart and reset_connection are special — they ack inline then restart the device.
    // These are one-time events where one blocking HTTP call is acceptable.
    if(messages.restart_device) {
        restart_device();  // acks then calls esp_restart() — never returns
        return;
    }

    if(messages.reset_connection) {
        Log.infoln("Message received: reset_connection");
        reset_connection();  // acks then calls esp_restart() — never returns
        return;
    }

    // For everything else: apply locally and queue acks for one-at-a-time processing

    if(messages.reset_eeprom) {
        Log.infoln("Message received: reset_eeprom");
        if(eepromManager.initializeEeprom()) {
            logInfo(INFO_EEPROM_INITIALIZED);
            settingsManager.loadSettings();
        }
        messages.reset_eeprom = false;
        pending_acks.reset_eeprom = true;
        // Discard dependent updates and queue their acks too
        if(messages.updated_cs) { messages.updated_cs = false; pending_acks.updated_cs = true; }
        if(messages.updated_cc) { messages.updated_cc = false; pending_acks.updated_cc = true; }
        if(messages.updated_mt) { messages.updated_mt = false; pending_acks.updated_mt = true; }
        if(messages.updated_devices) { messages.updated_devices = false; pending_acks.updated_devices = true; }
        force_full_config_send = true;
    }

    if(messages.default_cc) {
        Log.infoln("Message received: default_cc");
        TempControl::loadDefaultConstants();
        messages.default_cc = false;
        pending_acks.default_cc = true;
        // Cancel pending cc update
        messages.updated_cc = false;
        pending_acks.updated_cc = true;
    }

    if(messages.default_cs) {
        Log.infoln("Message received: default_cs");
        TempControl::loadDefaultSettings();
        messages.default_cs = false;
        pending_acks.default_cs = true;
        // Cancel pending cs update
        messages.updated_cs = false;
        pending_acks.updated_cs = true;
    }

    // Settings updates require an HTTP GET to fetch config — defer to fetch_and_apply_config()
    if(messages.updated_cs || messages.updated_cc || messages.updated_mt || messages.updated_devices) {
        needs_config_fetch = true;
    }

    if(messages.refresh_config) {
        messages.refresh_config = false;
        pending_acks.refresh_config = true;
        force_full_config_send = true;
    }
}

bool restHandler::ack_next_pending_message() {
    // Send one HTTP PATCH per call, in priority order. Returns true if an ack was sent.
    if(pending_acks.reset_eeprom) {
        set_message_processed(RestMessagesKeys::reset_eeprom);
        pending_acks.reset_eeprom = false;
        return true;
    }
    if(pending_acks.default_cc) {
        set_message_processed(RestMessagesKeys::default_cc);
        pending_acks.default_cc = false;
        return true;
    }
    if(pending_acks.default_cs) {
        set_message_processed(RestMessagesKeys::default_cs);
        pending_acks.default_cs = false;
        return true;
    }
    if(pending_acks.updated_cs) {
        set_message_processed(RestMessagesKeys::updated_cs);
        pending_acks.updated_cs = false;
        return true;
    }
    if(pending_acks.updated_cc) {
        set_message_processed(RestMessagesKeys::updated_cc);
        pending_acks.updated_cc = false;
        return true;
    }
    if(pending_acks.updated_mt) {
        set_message_processed(RestMessagesKeys::updated_mt);
        pending_acks.updated_mt = false;
        return true;
    }
    if(pending_acks.updated_devices) {
        set_message_processed(RestMessagesKeys::updated_devices);
        pending_acks.updated_devices = false;
        return true;
    }
    if(pending_acks.updated_es) {
        set_message_processed(RestMessagesKeys::updated_es);
        pending_acks.updated_es = false;
        return true;
    }
    if(pending_acks.refresh_config) {
        set_message_processed(RestMessagesKeys::refresh_config);
        pending_acks.refresh_config = false;
        return true;
    }
    return false;
}

bool restHandler::reset_eeprom() {
    // Now handled inline in apply_pending_messages()
    return true;
}

bool restHandler::reset_connection() {
    Log.infoln("Message received: reset_connection");
    messages.reset_connection = false;
    // Let the upstream know we processed this before we actually process it (since we'll (hopefully) disconnect)
    set_message_processed(RestMessagesKeys::reset_connection);

    // Reset the Fermentrack upstream settings
    upstreamSettings.setDefaults();
    upstreamSettings.storeToFilesystem();

    // Then disconnect WiFi and restart
    bp_wifi_disconnect(false);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return true;
}

bool restHandler::restart_device() {
    Log.infoln("Message received: restart_device");
    messages.restart_device = false;
    // Let the upstream know we processed this before we actually process it (since we'll (hopefully) disconnect)
    set_message_processed(RestMessagesKeys::restart_device);
    esp_restart();
    return true;
}

bool restHandler::default_cc() {
    // Now handled inline in apply_pending_messages()
    return true;
}

bool restHandler::default_cs() {
    // Now handled inline in apply_pending_messages()
    return true;
}


void load_settings_from_doc(JsonObject &root) {
    // Process
    for (JsonPair kv : root) {
        SettingLoader::processSettingKeypair(kv);
    }
}

bool restHandler::fetch_and_apply_config() {
    Log.infoln("Message received: updated_cs/cc/mt");

    std::string payload;
    char url[256] = "";
    std::string response;

    // We can't retrieve config if we're not registered
    if(upstreamSettings.isRegistered() == false) {
        needs_config_fetch = false;
        return false;
    }
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::fullConfig, upstreamSettings.deviceID, upstreamSettings.apiKey)) {
        needs_config_fetch = false;
        return false;
    }

    sendResult result = send_json_str(payload, url, response, httpMethod::HTTP_GET);
    if (result != sendResult::success) {
        needs_config_fetch = false;
        return false;
    }

    Log.verbose("Response: %s\r\n", response.c_str());

    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if(error) {
            Log.warning("deserializeJson() failed: %s\r\n", error.c_str());
            needs_config_fetch = false;
            return false;
        }

        if((doc["success"].is<bool>() && doc["success"].as<bool>() == false) || !doc["config"].is<JsonObject>()) {
            Log.warning("Error retrieving full config: ");
            Log.warningln(doc["message"].as<const char *>());
            needs_config_fetch = false;
            return false;
        }

        if(doc["config"]["cs"].is<JsonObject>() && messages.updated_cs) {
            Log.verboseln("Updating control settings");
            JsonObject root = doc["config"]["cs"].as<JsonObject>();
            load_settings_from_doc(root);
            TempControl::storeSettings();
            pending_acks.updated_cs = true;
        }

        if(doc["config"]["cc"].is<JsonObject>() && messages.updated_cc) {
            Log.verboseln("Updating control constants");
            JsonObject root = doc["config"]["cc"].as<JsonObject>();
            load_settings_from_doc(root);
            TempControl::storeConstants();
            pending_acks.updated_cc = true;
        }

        if(doc["config"]["devices"].is<JsonArray>() && messages.updated_devices) {
            Log.verboseln("Updating devices");
            JsonArray root = doc["config"]["devices"].as<JsonArray>();
            load_devices_from_array(root);
            pending_acks.updated_devices = true;
        }

        if(doc["config"]["mt"].is<JsonArray>() && messages.updated_mt) {
            Log.verboseln("Updating minimum times");
            // TODO - Write this
            pending_acks.updated_mt = true;
        }
    }

    // Clear message flags regardless of outcome, to avoid spamming the server
    messages.updated_cs = false;
    messages.updated_cc = false;
    messages.updated_devices = false;
    messages.updated_mt = false;

    needs_config_fetch = false;
    force_full_config_send = true;  // Always send a full config after processing an update

    return true;
}


void restHandler::load_devices_from_array(JsonArray &root) {
    // Process
    for (JsonDocument kv : root) {
        Log.verboseln("Processing device");
        DeviceDefinition dev;
        // JsonDocument doc;

        // serializeJsonPretty(kv, Serial);  // Print the received JSON to the console
        // piLink.receiveJsonMessage(doc);                                   // Read the JSON off the line from the Pi
        dev = DeviceManager::readJsonIntoDeviceDef(kv);                  // Parse the JSON into a DeviceDefinition object
        /*DeviceConfig print =*/ 
        deviceManager.updateDeviceDefinition(dev);   // Save the device definition (if valid)
        Log.verboseln("\r\nProcessed device.");
    }
}