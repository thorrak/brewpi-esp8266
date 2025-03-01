#ifndef BREWPI_HTTP_SERVER_H
#define BREWPI_HTTP_SERVER_H

#ifdef ENABLE_HTTP_INTERFACE

#define WEB_SERVER_PORT 80

#include <ESPAsyncWebServer.h>


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
    // WEBSERVER_IMPL *web_server;


private:
    void setStaticPages();
    void setJsonPages();
    void setPutPages();

    String getContentType(String filename);
    bool handleFileRead(AsyncWebServerRequest *request, String path);
    void redirect(AsyncWebServerRequest *request, const String &url);

};

extern httpServer http_server;

#endif //ENABLE_HTTP_INTERFACE

#endif //BREWPI_HTTP_SERVER_H
