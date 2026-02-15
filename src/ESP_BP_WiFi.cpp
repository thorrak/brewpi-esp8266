#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ESP_BP_WiFi.h"

#ifdef CONNECT_VIA_WIFI

#include <FS.h>  // Apparently this needs to be first
#include <string>
#include "Brewpi.h"

#if defined(ESP8266)
#include <ESP8266mDNS.h>
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#elif defined(ESP32)
#include <mdns.h>
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <esp_wifi.h>
#include <Ticks.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include "Version.h" 			// Used in mDNS announce string
#include "Display.h"
#include "EepromManager.h"
#include "rest/rest_send.h"


bool shouldSaveConfig = false;
int telnet_server_fd = -1;
int telnet_client_fd = -1;

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
    display.printWiFiStartup();
#ifdef ESP32
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);  // Set the bandwidth of ESP32 interface
#endif
}


// Not sure if this is sufficient to test for validity
bool isValidmDNSName(const char* mdns_name) {
    for (size_t i = 0; i < strlen(mdns_name); ++i) {
        // For now, we're just checking that every character in the string is alphanumeric. May need to add more validation here.
        if (!isalnum(mdns_name[i]))
            return false;
    }
    return true;
}

void mdns_reset() {
    std::string mdns_id;
    mdns_id = eepromManager.fetchmDNSName();

    mdns_free();

    if (mdns_init() == ESP_OK && mdns_hostname_set(mdns_id.c_str()) == ESP_OK) {
        mdns_txt_item_t txt[] = {
            {(char*)"board",    (char*)CONTROLLER_TYPE},
            {(char*)"branch",   (char*)"legacy"},
            {(char*)"version",  (char*)Config::Version::release},
            {(char*)"revision", (char*)FIRMWARE_REVISION},
        };
        mdns_service_add(NULL, "_brewpi", "_tcp", 23, txt, sizeof(txt) / sizeof(txt[0]));

        // if(Config::Prometheus::enable())
        //   mdns_service_add(NULL, "_brewpi_metrics", "_tcp", Config::Prometheus::port, NULL, 0);
    } else {
        // Serial.println("Error resetting mDNS responder.");
    }
}

void initWifiServer() {
    telnet_server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (telnet_server_fd < 0) return;

    int opt = 1;
    setsockopt(telnet_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // TCP_NODELAY equivalent of server.setNoDelay(true)
    setsockopt(telnet_server_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(23);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(telnet_server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(telnet_server_fd, 1);

    // Make server socket non-blocking
    int flags = fcntl(telnet_server_fd, F_GETFL, 0);
    fcntl(telnet_server_fd, F_SETFL, flags | O_NONBLOCK);

    mdns_reset();
}

#if defined(ESP8266)
// This doesn't work for ESP32, unfortunately
WiFiEventHandler stationConnectedHandler;
void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
    initWifiServer();
}
#endif

void initialize_wifi() {
    std::string mdns_id;
    WiFiManager wifiManager;

    display.clear();
    display.printWiFiConnect();  // TODO - Check if we have saved credentials before displaying this

    mdns_id = eepromManager.fetchmDNSName();

    WiFi.mode(WIFI_STA);        // explicitly set mode, esp defaults to STA+AP

#ifdef ESP8266
    WiFi.setOutputPower(20.5);  // Max transmit power
#endif

    wifiManager.setHostname(mdns_id.c_str());        // Allow DHCP to get proper name
    wifiManager.setWiFiAPChannel(1);         // Pick the most common channel, safe for all countries
    // Not sure if wm.SetCleanConnect breaks the hack we have below for the race condition - no reason to test it.
    // wifiManager.setCleanConnect(true);       // Always disconnect before connecting
    // wm.setCountry is causing crashes under Arduino Core 2.0, apparently
    wifiManager.setCountry("US");            // US country code is most restrictive, use for all countries


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
        // We succeeded at connecting
        // TODO - Determine if we can merge shouldSaveConfig in here
    } else {
        // If we failed to connect, we still want to control temps. Continue.
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
            vTaskDelay(pdMS_TO_TICKS(500));
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
    display.printWiFi();  // Print the WiFi info (mDNS name & IP address)
    vTaskDelay(pdMS_TO_TICKS(5000));
}


void wifi_connect_clients() {
    static unsigned long last_connection_check = 0;

    vTaskDelay(pdMS_TO_TICKS(1));
    if(WiFi.status() == WL_CONNECTED) {
        // We only accept clients if we do not have a REST target defined
        if(rest_handler.configured_for_fermentrack_rest()) {
            // If we have a telnet client connected, close it
            if (telnet_client_fd >= 0) {
                close(telnet_client_fd);
                telnet_client_fd = -1;
            }
        } else if (telnet_server_fd >= 0) {
            // Try to accept a new connection (non-blocking)
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_fd = accept(telnet_server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (new_fd >= 0) {
                // Close existing client if any
                if (telnet_client_fd >= 0) {
                    close(telnet_client_fd);
                }
                telnet_client_fd = new_fd;
                // Set TCP_NODELAY on client socket
                int opt = 1;
                setsockopt(telnet_client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
                // Make client socket non-blocking
                int flags = fcntl(telnet_client_fd, F_GETFL, 0);
                fcntl(telnet_client_fd, F_SETFL, flags | O_NONBLOCK);
            }
        }
    } else {
        // WiFi is disconnected -- close any telnet client
        if (telnet_client_fd >= 0) {
            close(telnet_client_fd);
            telnet_client_fd = -1;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1));

    // Additionally, every 3 minutes either attempt to reconnect WiFi, or rebroadcast mdns info
    if(ticks.millis() - last_connection_check >= (3 * 60 * 1000)) {
        last_connection_check = ticks.millis();
        if(WiFi.status() != WL_CONNECTED) {
            // If we are disconnected, reconnect. On an ESP8266 this will ALSO trigger mdns_reset due to the callback
            // but on the ESP32, this means that we'll have to wait an additional 3 minutes for mdns to come back up
            vTaskDelay(pdMS_TO_TICKS(150));
            WiFi.begin();
        } else {
#ifdef ESP32
            mdns_reset();  // TODO - Add this to the WiFi.reconnect() process
#endif
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
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
