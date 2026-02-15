#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ESP_BP_WiFi.h"

#include <esp_wifi.h>
#include <esp_netif.h>
#include <cstring>

#ifdef CONNECT_VIA_WIFI

#include <FS.h>  // Apparently this needs to be first
#include <string>
#include "Brewpi.h"

#include <mdns.h>
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <Ticks.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <fcntl.h>
#include <errno.h>

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
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);  // Set the bandwidth of ESP32 interface
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


void initialize_wifi() {
    std::string mdns_id;
    WiFiManager wifiManager;

    display.clear();
    display.printWiFiConnect();  // TODO - Check if we have saved credentials before displaying this

    mdns_id = eepromManager.fetchmDNSName();

    WiFi.mode(WIFI_STA);        // explicitly set mode, esp defaults to STA+AP

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
            bp_wifi_disconnect(true);
            vTaskDelay(pdMS_TO_TICKS(500));
            handleReset();
        }
    }
    // Auto-reconnect is handled by our own logic in wifi_connect_clients()
    // which re-attempts connection every 3 minutes when disconnected.
}

void wifi_connection_info(JsonDocument& doc) {
  doc["ssid"] = bp_wifi_get_ssid();
  doc["signalStrength"] = bp_wifi_get_rssi();
}

void display_connect_info_and_create_callback() {
    display.printWiFi();  // Print the WiFi info (mDNS name & IP address)
    vTaskDelay(pdMS_TO_TICKS(5000));
}


void wifi_connect_clients() {
    static unsigned long last_connection_check = 0;

    vTaskDelay(pdMS_TO_TICKS(1));
    if(bp_wifi_is_connected()) {
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
        if(!bp_wifi_is_connected()) {
            // If we are disconnected, reconnect.
            // We'll have to wait an additional 3 minutes for mdns to come back up
            vTaskDelay(pdMS_TO_TICKS(150));
            bp_wifi_reconnect();
        } else {
            mdns_reset();  // TODO - Add this to the WiFi.reconnect() process
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1));
}



#else
/*********************** Code for when we don't have WiFi enabled is below  *********************/

void initialize_wifi() {
    // Apparently, the WiFi radio is managed by the bootloader, so not including the libraries isn't the same as
    // disabling WiFi. We'll explicitly disable it if we're running in "serial" mode
    bp_wifi_off();
}

void display_connect_info_and_create_callback() {
    // For now, this is noop when WiFi support is disabled
}
void wifi_connect_clients() {
    // For now, this is noop when WiFi support is disabled
}
#endif


// -----------------------------------------------------------------------
// ESP-IDF WiFi utility functions (always compiled)
// -----------------------------------------------------------------------

bool bp_wifi_is_connected() {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) return false;

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) return false;

    return ip_info.ip.addr != 0;
}

const char* bp_wifi_get_ip_str() {
    static char ip_str[16]; // "xxx.xxx.xxx.xxx\0"

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) { strlcpy(ip_str, "0.0.0.0", sizeof(ip_str)); return ip_str; }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) { strlcpy(ip_str, "0.0.0.0", sizeof(ip_str)); return ip_str; }

    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    return ip_str;
}

uint32_t bp_wifi_get_ip_addr() {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) return 0;

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) return 0;

    return ip_info.ip.addr;
}

void bp_wifi_disconnect(bool erase_credentials) {
    esp_wifi_disconnect();

    if (erase_credentials) {
        // Clear the stored STA config so WiFiManager re-enters AP mode on next boot
        wifi_config_t conf;
        memset(&conf, 0, sizeof(conf));
        esp_wifi_set_config(WIFI_IF_STA, &conf);
    }
}

const char* bp_wifi_get_ssid() {
    static char ssid_buf[33]; // Max SSID length is 32 + null

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        strlcpy(ssid_buf, (const char *)ap_info.ssid, sizeof(ssid_buf));
    } else {
        ssid_buf[0] = '\0';
    }
    return ssid_buf;
}

int8_t bp_wifi_get_rssi() {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return 0;
}

const char* bp_wifi_get_hostname() {
    const char *hostname = nullptr;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_get_hostname(netif, &hostname);
    }
    return hostname ? hostname : "brewpi";
}

void bp_wifi_reconnect() {
    esp_wifi_connect();
}

void bp_wifi_off() {
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
}
