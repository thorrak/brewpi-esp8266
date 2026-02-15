#ifndef BREWPI_HTTP_SERVER_H
#define BREWPI_HTTP_SERVER_H

#ifdef ENABLE_HTTP_INTERFACE

#define WEB_SERVER_PORT 80

#include <esp_http_server.h>
#include <ArduinoJson.h>
#include "DeviceManager.h"  // For DeviceDefinition


class httpServer {
public:
    void init();

    bool lcd_reinit_rqd = false;
    bool restart_requested = false;
    bool name_reset_requested = false;
    bool wifi_reset_requested = false;
    bool config_reset_requested = false;
    bool ota_update_requested = false;
    bool device_definition_update_requested = false;

    DeviceDefinition dev;
    void processQueuedDeviceDefinition();
    void processQueuedActions();

    // JSON response/request helpers (public so handler templates can use them)
    static esp_err_t sendJsonDoc(httpd_req_t *req, JsonDocument &doc);
    static esp_err_t parseJsonBody(httpd_req_t *req, JsonDocument &doc);

private:
    httpd_handle_t server_handle = nullptr;

    void setStaticPages();
    void setJsonPages();
    void setPutPages();

    static const char* getContentType(const char* filename);
    static esp_err_t handleFileRead(httpd_req_t *req, const char* path);
    static esp_err_t static_file_handler(httpd_req_t *req);
    static esp_err_t not_found_handler(httpd_req_t *req, httpd_err_code_t err);
};

extern httpServer http_server;

#endif //ENABLE_HTTP_INTERFACE

#endif //BREWPI_HTTP_SERVER_H
