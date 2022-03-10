//
// Created by John Beeler on 1/12/20.
//

#include "ESP_BP_WiFi.h"

#ifdef ESP8266_WiFi

#include <FS.h>  // Apparently this needs to be first
#include "Brewpi.h"

#if defined(ESP8266)
#include <ESP8266mDNS.h>
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>	//Local WebServer used to serve the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#elif defined(ESP32)
#include <ESPmDNS.h>
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <esp_wifi.h>
#endif

#include "Version.h" 			// Used in mDNS announce string
#include "Display.h"
#include "EepromManager.h"
#include "EepromFormat.h"


bool shouldSaveConfig = false;
WiFiServer server(23);
WiFiClient serverClient;

extern void handleReset();  // Terrible practice. In brewpi-esp8266.cpp.


/**
 * \brief Callback notifying us of the need to save config
 * \ingroup wifi
 */
void saveConfigCallback() {
//    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void apCallback(WiFiManager *myWiFiManager) {
    // Callback to display the WiFi LCD notification and set bandwidth
    DisplayType::printWiFiStartup();
#ifdef ESP32
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);  // Set the bandwidth of ESP32 interface 
#endif
}


void initWifiServer() {
  server.begin();
  server.setNoDelay(true);
}

// Not sure if this is sufficient to test for validity
bool isValidmDNSName(const String& mdns_name) {
//    for (std::string::size_type i = 0; i < mdns_name.length(); ++i) {
    for (char i : mdns_name) {
        // For now, we're just checking that every character in the string is alphanumeric. May need to add more validation here.
        if (!isalnum(i))
            return false;
    }
    return true;
}


void mdns_reset() {
    String mdns_id;
    mdns_id = eepromManager.fetchmDNSName();

    MDNS.end();

    if (MDNS.begin(mdns_id.c_str())) {
        // mDNS will stop responding after awhile unless we query the specific service we want
        MDNS.addService("brewpi", "tcp", 23);
        MDNS.addServiceTxt("brewpi", "tcp", "board", CONTROLLER_TYPE);
        MDNS.addServiceTxt("brewpi", "tcp", "branch", "legacy");
        MDNS.addServiceTxt("brewpi", "tcp", "version", Config::Version::release);
        MDNS.addServiceTxt("brewpi", "tcp", "revision", FIRMWARE_REVISION);

        if(Config::Prometheus::enable())
          MDNS.addService("brewpi_metrics", "tcp", Config::Prometheus::port);
    } else {
        //Log.error(F("Error resetting MDNS responder."));
    }
}

void mdns_check() {
    char mdns_hostname[40];
    snprintf(mdns_hostname, 40, "%s.local", eepromManager.fetchmDNSName().c_str());
    if(!MDNS.queryHost(mdns_hostname, 2000)) {
        log_e("Lost mDNS Host - resetting");
        mdns_reset();
        return;
    }

    // As long as we can query the host, also check that something responds with the service we want
    MDNSResponder mdQuery;
    // int has_svc = mdQuery.queryService("brewpi", "tcp");
    if(mdQuery.queryService("brewpi", "tcp") == 0) {
        log_e("Lost mDNS Service - resetting");
        mdns_reset();
    }
}

#if defined(ESP8266)
// This doesn't work for ESP32, unfortunately
WiFiEventHandler stationConnectedHandler;
void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
    server.begin();
    server.setNoDelay(true);
    mdns_reset();
}
#endif

void initialize_wifi() {
    String mdns_id;
    WiFiManager wifiManager;

    mdns_id = eepromManager.fetchmDNSName();
//	if(mdns_id.length()<=0)
//		mdns_id = "ESP" + String(ESP.getChipId());


//    wifiManager.setHostname(config.mdnsID);  // Allow DHCP to get proper name
    wifiManager.setWiFiAutoReconnect(true);  // Enable auto reconnect (should remove need for reconnectWiFi())
    wifiManager.setWiFiAPChannel(1);         // Pick the most common channel, safe for all countries
    wifiManager.setCleanConnect(true);       // Always disconnect before connecting
    wifiManager.setCountry("US");            // US country code is most restrictive, use for all countries

    // If we're going to set up WiFi, let's get to it
    wifiManager.setConfigPortalTimeout(5*60); // Time out after 5 minutes so that we can keep managing temps
    wifiManager.setDebugOutput(false); // In case we have a serial connection to BrewPi

    wifiManager.setSaveParamsCallback(saveConfigCallback);
    wifiManager.setAPCallback(apCallback);                   // Set up when portal fires

    // The third parameter we're passing here (mdns_id.c_str()) is the default name that will appear on the form.
    // It's nice, but it means the user gets no actual prompt for what they're entering.
    WiFiManagerParameter custom_mdns_name("mdns", "Device (mDNS) Name", mdns_id.c_str(), 20);
    wifiManager.addParameter(&custom_mdns_name);


    if(wifiManager.autoConnect(WIFI_SETUP_AP_NAME, WIFI_SETUP_AP_PASS)) {
        // If we succeeded at connecting, switch to station mode.
        // TODO - Determine if we can merge shouldSaveConfig in here
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_AP_STA);
    } else {
        // If we failed to connect, we still want to control temps. Disable the AP, and flip to STA mode
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_AP_STA);
    }

    // Alright. We're theoretically connected here (or we timed out).
    // If we connected, then let's save the mDNS name
    if (shouldSaveConfig) {
        // If the mDNS name is valid, save it.
        if (isValidmDNSName(custom_mdns_name.getValue())) {
            eepromManager.savemDNSName(custom_mdns_name.getValue());
        } else {
            // If the mDNS name is invalid, reset the WiFi configuration and restart the ESP8266
            WiFi.disconnect(true);
            delay(2000);
            handleReset();
        }
    }

    // Regardless of the above, we need to set the mDNS name and announce it
    mdns_reset();

    // This will trigger autoreconnection, but will not connect if we aren't connected at this point (e.g. if the AP is
    // not yet broadcasting)
    WiFi.setAutoReconnect(true);
}

void wifi_connection_info(JsonDocument& doc) {
  doc["ssid"] = WiFi.SSID();
  doc["signalStrength"] = WiFi.RSSI();
}

void display_connect_info_and_create_callback() {
#if defined(ESP8266)
    // This doesn't work for ESP32, unfortunately.
    stationConnectedHandler = WiFi.onSoftAPModeStationConnected(&onStationConnected);
#endif
    DisplayType::printWiFi();  // Print the WiFi info (mDNS name & IP address)
    delay(5000);
}


void wifi_connect_clients() {
    static unsigned long last_connection_check = 0;

    yield();
    if(WiFi.isConnected()) {
        if (server.hasClient()) {
            // If we show a client as already being disconnected, force a disconnect
            if (serverClient) serverClient.stop();
            serverClient = server.available();
            serverClient.flush();
        }
    } else {
        // This might be unnecessary, but let's go ahead and disconnect any
        // "clients" we show as connected given that WiFi isn't connected. If
        // we show a client as already being disconnected, force a disconnect
        if (serverClient) {
            serverClient.stop();
            serverClient = server.available();
            serverClient.flush();
        }
    }
    yield();

    // Additionally, every 3 minutes either attempt to reconnect WiFi, or rebroadcast mdns info
    if(ticks.millis() - last_connection_check >= (3 * 60 * 1000)) {
        last_connection_check = ticks.millis();
        if(!WiFi.isConnected()) {
            // If we are disconnected, reconnect. On an ESP8266 this will ALSO trigger mdns_reset due to the callback
            // but on the ESP32, this means that we'll have to wait an additional 3 minutes for mdns to come back up
            WiFi.reconnect();
        } else {
            // Commenting these out for now as there is a memory leak caused by this
            // mdns_reset();
            // mdns_check();
        }
    }
    yield();
}



#else
/*********************** Code for when we don't have WiFi enabled is below  *********************/

void initialize_wifi() {
    // Apparently, the WiFi radio is managed by the bootloader, so not including the libraries isn't the same as
    // disabling WiFi. We'll explicitly disable it if we're running in "serial" mode
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void display_connect_info_and_create_callback() {
    // For now, this is noop when WiFi support is disabled
}
void wifi_connect_clients() {
    // For now, this is noop when WiFi support is disabled
}
#endif
