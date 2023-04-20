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
    if(messages.updated_cs || messages.updated_cc || messages.updated_mt) {
        process_updated_settings();
    }




    // Process resetting WiFi last, as we will lose the ability to signal that we processed it
    if(messages.reset_wifi) {
        Serial.println("Message received: reset_wifi");
        reset_wifi();
    }

    // bool updated_es = false;
    // bool updated_devices = false;


}

bool restHandler::reset_eeprom() {
    Serial.println("Message received: reset_eeprom");
    if(eepromManager.initializeEeprom()) {
        logInfo(INFO_EEPROM_INITIALIZED);
        settingsManager.loadSettings();
    }
    messages.reset_eeprom = false;
    set_message_processed(RestMessagesKeys::reset_eeprom);

    // If we reset the EEPROM, we discard any pending messages/updates
    set_message_processed(RestMessagesKeys::updated_cs);
    set_message_processed(RestMessagesKeys::updated_cc);
    set_message_processed(RestMessagesKeys::updated_mt);
    set_message_processed(RestMessagesKeys::updated_devices);

    messages.updated_cs = false;
    messages.updated_cc = false;
    messages.updated_mt = false;
    messages.updated_devices = false;

    // ...also, trigger a send of settings next time we can
    send_full_config_ticker = true;

    return true;
}

bool restHandler::reset_wifi() {
    Serial.println("Message received: reset_wifi");
    messages.reset_wifi = false;
    // Let the upstream know we processed this before we actually process it (since we'll (hopefully) disconnect)
    set_message_processed(RestMessagesKeys::reset_wifi);
    WiFi.disconnect(true);
    // TODO - Decide if we want to restart here
    return true;
}

bool restHandler::restart_device() {
    Serial.println("Message received: restart_device");
    messages.restart_device = false;
    // Let the upstream know we processed this before we actually process it (since we'll (hopefully) disconnect)
    set_message_processed(RestMessagesKeys::restart_device);
    ESP.restart();
    return true;
}

bool restHandler::default_cc() {
    Serial.println("Message received: default_cc");

    TempControl::loadDefaultConstants();

    messages.default_cc = false;
    set_message_processed(RestMessagesKeys::default_cc);

    // Also, cancel any pending cc update, as we just defaulted them
    messages.updated_cc = false;
    set_message_processed(RestMessagesKeys::updated_cc);

    return true;
}

bool restHandler::default_cs() {
    Serial.println("Message received: default_cs");

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
    Serial.println("Message received: updated_cs/cc/mt");

    String payload = "";
    char url[256] = "";
    String response;

    // We can't retrieve config if we're not registered
    if(upstreamSettings.isRegistered() == false)
        return false;
    if(!get_url(url, sizeof(url), UpstreamAPIEndpoints::fullConfig, upstreamSettings.deviceID, upstreamSettings.apiKey))
        return false;

    send_json_str(payload, url, response, httpMethod::HTTP_GET);
    Serial.printf("Response: %s\r\n", response.c_str());

    {
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, response);

        if(error) {
            Serial.print(F("deserializeJson() failed: "));
            return false;
        }

        if((doc.containsKey("success") && doc["success"].as<bool>() == false) || !doc.containsKey("config")) {
            Serial.print(F("Error retrieving full config: "));
            Serial.println(doc["message"].as<String>());
            return false;
        }

        if(doc["config"].containsKey("cs") && messages.updated_cs) {
            Serial.println("Updating control settings");
            JsonObject root = doc["config"]["cs"].as<JsonObject>();
            load_settings_from_doc(root);
            TempControl::storeSettings();

        }

        if(doc["config"].containsKey("cc") && messages.updated_cc) {
            Serial.println("Updating control constants");
            JsonObject root = doc["config"]["cc"].as<JsonObject>();
            load_settings_from_doc(root);
            TempControl::storeConstants();

        }

    }
    messages.updated_cs = false;
    set_message_processed(RestMessagesKeys::updated_cs);
    messages.updated_cc = false;
    set_message_processed(RestMessagesKeys::updated_cc);
    messages.updated_mt = false;
    set_message_processed(RestMessagesKeys::updated_mt);

    return true;
}
