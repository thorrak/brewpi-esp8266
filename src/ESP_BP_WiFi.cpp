#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ESP_BP_WiFi.h"

#include <esp_wifi.h>
#include <esp_netif.h>
#include <cstring>

#ifdef CONNECT_VIA_WIFI

#include <string>
#include "Brewpi.h"

#include <mdns.h>
#include <esp_event.h>
#include <freertos/event_groups.h>
#include <esp_http_server.h>
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


int telnet_server_fd = -1;
int telnet_client_fd = -1;

extern void handleReset();  // Terrible practice. In brewpi-esp8266.cpp.

// WiFi event group bits
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_count = 0;
static const int MAX_RETRIES = 4;
static const int CONNECT_TIMEOUT_MS = 10000;

// Captive portal state
static int s_dns_socket = -1;
static httpd_handle_t s_portal_server = nullptr;
static volatile bool s_portal_config_received = false;
static char s_portal_ssid[33] = {0};
static char s_portal_pass[64] = {0};
static char s_portal_mdns[21] = {0};


/**
 * \brief WiFi event handler for STA connect/disconnect and IP acquisition
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < MAX_RETRIES) {
            s_retry_count++;
            printf("WiFi disconnected, retry %d/%d\n", s_retry_count, MAX_RETRIES);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("WiFi connected, IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
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


// -----------------------------------------------------------------------
// Captive portal: DNS server
// -----------------------------------------------------------------------

/**
 * \brief Minimal DNS server task that responds to ALL queries with the AP IP.
 *
 * Runs as a FreeRTOS task. Listens on UDP port 53 and replies with a
 * hard-coded A record pointing to 192.168.4.1 regardless of the query.
 */
static void dns_server_task(void* param) {
    s_dns_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_dns_socket < 0) {
        printf("Captive portal: failed to create DNS socket\n");
        vTaskDelete(nullptr);
        return;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s_dns_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Captive portal: failed to bind DNS socket\n");
        close(s_dns_socket);
        s_dns_socket = -1;
        vTaskDelete(nullptr);
        return;
    }

    // Set a 1-second receive timeout so we can check for task deletion
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(s_dns_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint8_t buf[512];
    while (s_dns_socket >= 0) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int len = recvfrom(s_dns_socket, buf, sizeof(buf), 0,
                           (struct sockaddr*)&client_addr, &addr_len);
        if (len < 12) continue; // Too short or timeout

        // Build a minimal DNS response:
        // Copy the query, set response flags, append an A record answer
        uint8_t resp[512];
        memcpy(resp, buf, len);

        // Set QR bit (response), no error
        resp[2] = 0x81; // QR=1, Opcode=0, AA=1
        resp[3] = 0x80; // RA=1, RCODE=0
        // Set answer count = 1
        resp[6] = 0x00;
        resp[7] = 0x01;

        // Append answer section after the query
        int pos = len;
        // Name pointer to the query name (offset 12)
        resp[pos++] = 0xC0;
        resp[pos++] = 0x0C;
        // Type A
        resp[pos++] = 0x00;
        resp[pos++] = 0x01;
        // Class IN
        resp[pos++] = 0x00;
        resp[pos++] = 0x01;
        // TTL = 60 seconds
        resp[pos++] = 0x00;
        resp[pos++] = 0x00;
        resp[pos++] = 0x00;
        resp[pos++] = 0x3C;
        // Data length = 4
        resp[pos++] = 0x00;
        resp[pos++] = 0x04;
        // IP: 192.168.4.1
        resp[pos++] = 192;
        resp[pos++] = 168;
        resp[pos++] = 4;
        resp[pos++] = 1;

        sendto(s_dns_socket, resp, pos, 0,
               (struct sockaddr*)&client_addr, addr_len);
    }

    vTaskDelete(nullptr);
}

static void stop_dns_server() {
    if (s_dns_socket >= 0) {
        int fd = s_dns_socket;
        s_dns_socket = -1; // Signal the task to exit
        close(fd);
    }
}


// -----------------------------------------------------------------------
// Captive portal: HTTP config page
// -----------------------------------------------------------------------

static const char PORTAL_HTML[] =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>BrewPi WiFi Setup</title>"
    "<style>"
    "body{font-family:sans-serif;margin:20px;background:#f0f0f0;}"
    "form{background:#fff;padding:20px;border-radius:8px;max-width:320px;margin:auto;}"
    "h2{text-align:center;color:#333;}"
    "label{display:block;margin-top:12px;font-weight:bold;color:#555;}"
    "input[type=text],input[type=password]{width:100%%;padding:8px;margin-top:4px;"
    "border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}"
    "input[type=submit]{width:100%%;padding:10px;margin-top:16px;background:#2196F3;"
    "color:#fff;border:none;border-radius:4px;font-size:16px;cursor:pointer;}"
    "</style></head><body>"
    "<form action='/save' method='POST'>"
    "<h2>BrewPi WiFi Setup</h2>"
    "<label>WiFi Network (SSID)</label>"
    "<input type='text' name='ssid' maxlength='32' required>"
    "<label>WiFi Password</label>"
    "<input type='password' name='pass' maxlength='63'>"
    "<label>Device (mDNS) Name</label>"
    "<input type='text' name='mdns' maxlength='20' value='%s'>"
    "<input type='submit' value='Save &amp; Connect'>"
    "</form></body></html>";

static const char PORTAL_SUCCESS_HTML[] =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>BrewPi WiFi Setup</title>"
    "<style>body{font-family:sans-serif;text-align:center;margin-top:40px;}</style>"
    "</head><body>"
    "<h2>Settings Saved</h2>"
    "<p>BrewPi will now restart and connect to your WiFi network.</p>"
    "</body></html>";


/**
 * \brief URL-decode a string in place. Handles %XX and '+' -> ' '.
 */
static void url_decode(char* dst, const char* src, size_t dst_size) {
    size_t di = 0;
    for (size_t si = 0; src[si] && di < dst_size - 1; si++) {
        if (src[si] == '+') {
            dst[di++] = ' ';
        } else if (src[si] == '%' && src[si+1] && src[si+2]) {
            char hex[3] = { src[si+1], src[si+2], 0 };
            dst[di++] = (char)strtol(hex, nullptr, 16);
            si += 2;
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

/**
 * \brief Extract a named parameter from URL-encoded form body.
 */
static bool extract_param(const char* body, const char* name, char* out, size_t out_size) {
    char search[64];
    snprintf(search, sizeof(search), "%s=", name);
    const char* start = strstr(body, search);
    if (!start) {
        out[0] = '\0';
        return false;
    }
    start += strlen(search);
    const char* end = strchr(start, '&');
    size_t len = end ? (size_t)(end - start) : strlen(start);

    // Copy the raw value, then decode
    char raw[128];
    if (len >= sizeof(raw)) len = sizeof(raw) - 1;
    memcpy(raw, start, len);
    raw[len] = '\0';
    url_decode(out, raw, out_size);
    return true;
}

static esp_err_t portal_get_handler(httpd_req_t* req) {
    std::string mdns_name = eepromManager.fetchmDNSName();
    char* html = (char*)malloc(sizeof(PORTAL_HTML) + 32);
    if (!html) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    snprintf(html, sizeof(PORTAL_HTML) + 32, PORTAL_HTML, mdns_name.c_str());
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    free(html);
    return ESP_OK;
}

static esp_err_t portal_save_handler(httpd_req_t* req) {
    char body[256] = {0};
    int received = httpd_req_recv(req, body, sizeof(body) - 1);
    if (received <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    body[received] = '\0';

    extract_param(body, "ssid", s_portal_ssid, sizeof(s_portal_ssid));
    extract_param(body, "pass", s_portal_pass, sizeof(s_portal_pass));
    extract_param(body, "mdns", s_portal_mdns, sizeof(s_portal_mdns));

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, PORTAL_SUCCESS_HTML, strlen(PORTAL_SUCCESS_HTML));

    s_portal_config_received = true;
    return ESP_OK;
}

/**
 * \brief Catch-all handler that redirects any unknown URI to the portal root.
 * This makes the captive portal work on most devices/OSes.
 */
static esp_err_t portal_redirect_handler(httpd_req_t* req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

static httpd_handle_t start_portal_http_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 4;

    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &config) != ESP_OK) {
        printf("Captive portal: failed to start HTTP server\n");
        return nullptr;
    }

    // GET /
    httpd_uri_t root_uri = {};
    root_uri.uri = "/";
    root_uri.method = HTTP_GET;
    root_uri.handler = portal_get_handler;
    httpd_register_uri_handler(server, &root_uri);

    // POST /save
    httpd_uri_t save_uri = {};
    save_uri.uri = "/save";
    save_uri.method = HTTP_POST;
    save_uri.handler = portal_save_handler;
    httpd_register_uri_handler(server, &save_uri);

    // Catch-all: redirect to root (for captive portal detection)
    httpd_uri_t catchall_uri = {};
    catchall_uri.uri = "/*";
    catchall_uri.method = HTTP_GET;
    catchall_uri.handler = portal_redirect_handler;
    httpd_register_uri_handler(server, &catchall_uri);

    return server;
}

static void stop_portal_http_server() {
    if (s_portal_server) {
        httpd_stop(s_portal_server);
        s_portal_server = nullptr;
    }
}


// -----------------------------------------------------------------------
// Captive portal: start / stop
// -----------------------------------------------------------------------

static void start_captive_portal() {
    printf("WiFi: starting captive portal AP\n");

    // Switch to AP+STA mode
    esp_wifi_set_mode(WIFI_MODE_APSTA);

    // Configure the AP
    wifi_config_t ap_config = {};
    strlcpy((char*)ap_config.ap.ssid, WIFI_SETUP_AP_NAME, sizeof(ap_config.ap.ssid));
    strlcpy((char*)ap_config.ap.password, WIFI_SETUP_AP_PASS, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len = strlen(WIFI_SETUP_AP_NAME);
    ap_config.ap.channel = 1;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.max_connection = 4;
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);

    display.printWiFiStartup();

    // Start DNS server task
    xTaskCreate(dns_server_task, "dns_srv", 4096, nullptr, 5, nullptr);

    // Start HTTP server for config page
    s_portal_server = start_portal_http_server();
}

static void stop_captive_portal() {
    stop_dns_server();
    stop_portal_http_server();

    // Switch back to STA-only mode
    esp_wifi_set_mode(WIFI_MODE_STA);
    printf("WiFi: captive portal stopped\n");
}


// -----------------------------------------------------------------------
// initialize_wifi()
// -----------------------------------------------------------------------

void initialize_wifi() {
    std::string mdns_id;

    display.clear();
    display.printWiFiConnect();

    mdns_id = eepromManager.fetchmDNSName();

    // Create the event group
    s_wifi_event_group = xEventGroupCreate();

    // Initialize the TCP/IP stack and create default STA + AP netifs
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    // Set hostname before connecting so DHCP gets the right name
    esp_netif_set_hostname(sta_netif, mdns_id.c_str());

    // Initialize WiFi driver with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Register event handlers
    esp_event_handler_instance_t instance_wifi;
    esp_event_handler_instance_t instance_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, nullptr, &instance_wifi);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, nullptr, &instance_ip);

    // Set STA mode and start WiFi
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // Attempt connection using stored NVS credentials (compatible with previous WiFiManager creds)
    s_retry_count = 0;
    esp_wifi_connect();

    // Wait for connection or failure
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(CONNECT_TIMEOUT_MS * MAX_RETRIES));

    if (bits & WIFI_CONNECTED_BIT) {
        printf("WiFi: connected to stored network\n");
    } else {
        // Connection failed -- start captive portal for configuration
        printf("WiFi: connection to stored network failed, starting captive portal\n");

        s_portal_config_received = false;
        start_captive_portal();

        // Wait up to 5 minutes for the user to submit config
        const unsigned long portal_start = xTaskGetTickCount();
        const TickType_t portal_timeout = pdMS_TO_TICKS(5 * 60 * 1000);

        while (!s_portal_config_received &&
               (xTaskGetTickCount() - portal_start) < portal_timeout) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        stop_captive_portal();

        if (s_portal_config_received) {
            printf("WiFi: received config from portal (SSID=%s)\n", s_portal_ssid);

            // Save mDNS name if valid
            if (strlen(s_portal_mdns) > 0 && isValidmDNSName(s_portal_mdns)) {
                eepromManager.savemDNSName(s_portal_mdns);
            } else if (strlen(s_portal_mdns) > 0) {
                // Invalid mDNS name -- erase creds and restart
                bp_wifi_disconnect(true);
                vTaskDelay(pdMS_TO_TICKS(500));
                handleReset();
            }

            // Store the new WiFi credentials via esp_wifi_set_config (persisted to NVS)
            wifi_config_t sta_config = {};
            strlcpy((char*)sta_config.sta.ssid, s_portal_ssid, sizeof(sta_config.sta.ssid));
            strlcpy((char*)sta_config.sta.password, s_portal_pass, sizeof(sta_config.sta.password));
            esp_wifi_set_config(WIFI_IF_STA, &sta_config);

            // Attempt to connect with the new credentials
            s_retry_count = 0;
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
            esp_wifi_connect();

            bits = xEventGroupWaitBits(s_wifi_event_group,
                                       WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                       pdTRUE, pdFALSE,
                                       pdMS_TO_TICKS(CONNECT_TIMEOUT_MS * MAX_RETRIES));

            if (bits & WIFI_CONNECTED_BIT) {
                printf("WiFi: connected with new credentials\n");
            } else {
                printf("WiFi: failed to connect with new credentials, continuing without WiFi\n");
            }
        } else {
            printf("WiFi: captive portal timed out, continuing without WiFi\n");
        }
    }

    // Unregister the init-time event handlers -- reconnection is managed
    // by wifi_connect_clients() calling bp_wifi_reconnect()
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_wifi);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_ip);

    vEventGroupDelete(s_wifi_event_group);
    s_wifi_event_group = nullptr;
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
        // Clear the stored STA config so we re-enter AP mode on next boot
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
