#include "ESP_BP_WiFi.h"

#ifdef ESP8266_WiFi

#include <FS.h>  // Apparently this needs to be first
#include "Brewpi.h"

#if defined(ESP8266)
#include <ESP8266mDNS.h>
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#elif defined(ESP32)
#include <ESPmDNS.h>
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <esp_wifi.h>
#include <Ticks.h>
#endif

#include "Version.h" 			// Used in mDNS announce string
#include "Display.h"
#include "EepromManager.h"


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

const char * mdns_servicename = "brewpi";

void mdns_reset() {
    String mdns_id;
    mdns_id = eepromManager.fetchmDNSName();

    MDNS.end();  // TODO - Determine if we still need to do this, given the addition of mDNS.update() to the loop

    if (MDNS.begin(mdns_id.c_str())) {
        // mDNS will stop responding after awhile unless we query the specific service we want
        MDNS.addService(mdns_servicename, "tcp", 23);
        MDNS.addServiceTxt(mdns_servicename, "tcp", "board", CONTROLLER_TYPE);
        MDNS.addServiceTxt(mdns_servicename, "tcp", "branch", "legacy");
        MDNS.addServiceTxt(mdns_servicename, "tcp", "version", Config::Version::release);
        MDNS.addServiceTxt(mdns_servicename, "tcp", "revision", FIRMWARE_REVISION);

        // if(Config::Prometheus::enable())
        //   MDNS.addService("brewpi_metrics", "tcp", Config::Prometheus::port);
    } else {
        // Serial.println("Error resetting mDNS responder.");
    }
}

void initWifiServer() {
  server.begin();
  server.setNoDelay(true);
    mdns_reset();
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

    display.clear();
    display.printWiFiConnect();  // TODO - Check if we have saved credentials before displaying this

    mdns_id = eepromManager.fetchmDNSName();

    WiFi.mode(WIFI_STA);        // explicitly set mode, esp defaults to STA+AP

#ifdef ESP8266
    WiFi.setOutputPower(20.5);  // Max transmit power
#endif

    wifiManager.setHostname(mdns_id);        // Allow DHCP to get proper name
    wifiManager.setWiFiAutoReconnect(true);  // Enable auto reconnect (should remove need for reconnectWiFi())
    wifiManager.setWiFiAPChannel(1);         // Pick the most common channel, safe for all countries
    // Not sure if wm.SetCleanConnect breaks the hack we have below for the race condition - no reason to test it.
    // wifiManager.setCleanConnect(true);       // Always disconnect before connecting
    // wm.setCountry is causing crashes under Arduino Core 2.0, apparently
    // wifiManager.setCountry("US");            // US country code is most restrictive, use for all countries


    // There is a race condition on some routers when processing the deauthorization that the ESP attempts as it
    // connects. This can result in it seeming like every other connection attempt works, or only connection
    // attempts after a hard reset. One way around this bug is to just reattempt connection multiple times until
    // it takes. 
    wifiManager.setConnectTimeout(10);
    // sets number of retries for autoconnect, force retry after wait failure exit
    wifiManager.setConnectRetries(4); // default 1

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
            // TODO - Set hostname here again (in case it changed)
            eepromManager.savemDNSName(custom_mdns_name.getValue());
        } else {
            // If the mDNS name is invalid, reset the WiFi configuration and restart the device
            WiFi.disconnect(true);
            delay(2000);
            handleReset();
        }
    }
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
            // #defining this out for now as there is a memory leak caused by this
#ifdef ESP32
            mdns_reset();  // TODO - Add this to the WiFi.reconnect() process
#endif
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
