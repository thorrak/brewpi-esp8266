#ifndef BREWPI_HTTP_SERVER_H
#define BREWPI_HTTP_SERVER_H

#ifdef ENABLE_HTTP_INTERFACE

#define WEB_SERVER_PORT 80


uint8_t processUpstreamConfigUpdateJson(const JsonDocument& json, bool triggerUpstreamUpdate = true);


class httpServer {
public:
    void init();
    //void handleClient();
    bool lcd_reinit_rqd = false;
    bool restart_requested = false;
    bool name_reset_requested = false;
    bool wifi_reset_requested = false;
    bool config_reset_requested = false;
    bool ota_update_requested = false;
};

extern httpServer http_server;

#endif //ENABLE_HTTP_INTERFACE

#endif //TRONBRIDGE_HTTP_SERVER_H
