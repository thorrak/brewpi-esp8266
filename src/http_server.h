#ifndef BREWPI_HTTP_SERVER_H
#define BREWPI_HTTP_SERVER_H

#ifdef ENABLE_HTTP_INTERFACE

#define WEB_SERVER_PORT 80

#ifdef ESP8266
#include <ESP8266WebServer.h>
#define WEBSERVER_IMPL ESP8266WebServer
#elif defined(ESP32)
#include <WebServer.h>
#define WEBSERVER_IMPL WebServer
#endif


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
    WEBSERVER_IMPL *web_server;


private:
    void uptime();
    void heap();
    void genericServeJson(void(*jsonFunc)(JsonDocument&));
    void processJsonRequest(const char* uri, uint8_t (*handler)(const JsonDocument& json, bool triggerUpstreamUpdate));
    void serveExtendedSettings();
    void serveUpstreamSettings();
    void serveMinTimes();
    void reset_reason();

    void setStaticPages();
    void setJsonPages();
    void setJsonHandlers();

    String getContentType(String filename);
    // bool exists(String path);
    bool handleFileRead(String path);
    void redirect(const String& url);

};

extern httpServer http_server;

#endif //ENABLE_HTTP_INTERFACE

#endif //TRONBRIDGE_HTTP_SERVER_H
