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
#include "SettingsManager.h"
#include "SettingLoader.h"


void restHandler::process_messages() {


    // Restarting the device is second to last
    if(messages.restart_device) {
        restart_device();
    }

    // If we have an EEPROM reset, process that first (as it will cancel processing cs/cc/mt/devices)
    if(messages.reset_eeprom) {
        reset_eeprom();
    }

    // Additionally, proces default_cc/cs as those will cancel processing their respective messages
    if(messages.default_cc) {
        default_cc();
    }

    if(messages.default_cs) {
        default_cs();
    }

    // Next, process updated_cc/cs/mt/devices
    if(messages.updated_cs || messages.updated_cc || messages.updated_mt || messages.updated_devices) {
        process_updated_settings();
    }


    // Next, process refresh config request
    if(messages.refresh_config) {
        send_full_config_ticker = true;
        set_message_processed(RestMessagesKeys::refresh_config);
        messages.refresh_config = false;
    }

    // Process resetting WiFi last, as we will lose the ability to signal that we processed it
    if(messages.reset_connection) {
        Log.infoln("Message received: reset_connection");
        reset_connection();
    }

    // bool updated_es = false;
    // bool updated_devices = false;


}

bool restHandler::reset_eeprom() {
    Log.infoln("Message received: reset_eeprom");
    if(eepromManager.initializeEeprom()) {
        logInfo(INFO_EEPROM_INITIALIZED);
        settingsManager.loadSettings();
    }
    messages.reset_eeprom = false;
    set_message_processed(RestMessagesKeys::reset_eeprom);

    // If we reset the EEPROM, we discard any pending messages/updates
    if(messages.updated_cs)
        set_message_processed(RestMessagesKeys::updated_cs);
    if(messages.updated_cc)
        set_message_processed(RestMessagesKeys::updated_cc);
    if(messages.updated_mt)
        set_message_processed(RestMessagesKeys::updated_mt);
    if(messages.updated_devices)
        set_message_processed(RestMessagesKeys::updated_devices);

    messages.updated_cs = false;
    messages.updated_cc = false;
    messages.updated_mt = false;
    messages.updated_devices = false;

    // ...also, trigger a send of settings next time we can
    send_full_config_ticker = true;

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
    WiFi.disconnect(false, true);
    delay(500);
    ESP.restart();
    return true;
}

bool restHandler::restart_device() {
    Log.infoln("Message received: restart_device");
    messages.restart_device = false;
    // Let the upstream know we processed this before we actually process it (since we'll (hopefully) disconnect)
    set_message_processed(RestMessagesKeys::restart_device);
    ESP.restart();
    return true;
}

bool restHandler::default_cc() {
    Log.infoln("Message received: default_cc");

    TempControl::loadDefaultConstants();

    messages.default_cc = false;
    set_message_processed(RestMessagesKeys::default_cc);

    // Also, cancel any pending cc update, as we just defaulted them
    messages.updated_cc = false;
    set_message_processed(RestMessagesKeys::updated_cc);

    return true;
}

bool restHandler::default_cs() {
    Log.infoln("Message received: default_cs");

    TempControl::loadDefaultSettings();

    messages.default_cs = false;
    set_message_processed(RestMessagesKeys::default_cs);

    // Also, cancel any pending cs update, as we just defaulted them
    messages.updated_cs = false;
    set_message_processed(RestMessagesKeys::updated_cs);

    return true;
}


void load_settings_from_doc(JsonObject &root) {
    // Process
    for (JsonPair kv : root) {
        SettingLoader::processSettingKeypair(kv);
    }
}

bool restHandler::process_updated_settings() {
    Log.infoln("Message received: updated_cs/cc/mt");

    String payload = "";
    char url[256] = "";
    String response;

    // We can't retrieve config if we're not registered
    if(upstreamSettings.isRegistered() == false)
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::fullConfig, upstreamSettings.deviceID, upstreamSettings.apiKey))
        return false;

    send_json_str(payload, url, response, httpMethod::HTTP_GET);
    Log.verbose("Response: %s\r\n", response.c_str());

    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if(error) {
            Log.warning("deserializeJson() failed: %s\r\n", error.c_str());
            return false;
        }

        if((doc["success"].is<bool>() && doc["success"].as<bool>() == false) || !doc["config"].is<JsonObject>()) {
            Log.warning(F("Error retrieving full config: "));
            Log.warningln(doc["message"].as<String>());
            return false;
        }

        if(doc["config"]["cs"].is<JsonObject>() && messages.updated_cs) {
            Log.verboseln("Updating control settings");
            JsonObject root = doc["config"]["cs"].as<JsonObject>();
            load_settings_from_doc(root);
            TempControl::storeSettings();
            set_message_processed(RestMessagesKeys::updated_cs);
        }

        if(doc["config"]["cc"].is<JsonObject>() && messages.updated_cc) {
            Log.verboseln("Updating control constants");
            JsonObject root = doc["config"]["cc"].as<JsonObject>();
            load_settings_from_doc(root);
            TempControl::storeConstants();
            set_message_processed(RestMessagesKeys::updated_cc);
        }

        if(doc["config"]["devices"].is<JsonArray>() && messages.updated_devices) {
            Log.verboseln("Updating devices");
            JsonArray root = doc["config"]["devices"].as<JsonArray>();
            load_devices_from_array(root);
            // TempControl::storeConstants();
            set_message_processed(RestMessagesKeys::updated_devices);
        }

        if(doc["config"]["mt"].is<JsonArray>() && messages.updated_mt) {
            Log.verboseln("Updating minimum times");
            // TODO - Write this
            // JsonObject root = doc["config"]["cc"].as<JsonObject>();
            // load_settings_from_doc(root);
            set_message_processed(RestMessagesKeys::updated_mt);
        }
    }

    // We clear the flag locally in every case, so as to not spam the server if something goes wrong
    messages.updated_cs = false;
    messages.updated_cc = false;
    messages.updated_devices = false;
    messages.updated_mt = false;

    rest_handler.send_full_config_ticker=true;  // We always want to send a full config after processing an update

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