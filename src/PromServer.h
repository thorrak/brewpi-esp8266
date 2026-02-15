#pragma once
#ifdef ENABLE_PROMETHEUS_SERVER
#include "Brewpi.h"
#include "TemperatureFormats.h"
#include "Ticks.h"
#include <string>
#include <esp_http_server.h>

class PromServer {
public:
    void setup();
    static void invalidateCache();

private:
    httpd_handle_t server = nullptr;

    static esp_err_t metricsHandler(httpd_req_t *req);

    static const char metricsTemplate[];
    static const char probeTemplate[];

    static std::string formatProbeTemp(const temperature temp);
    static std::string probeValues();
    static std::string processTemplate();

    static std::string probeCache;
    static constexpr auto cacheTime = 300;
    static ticks_seconds_t dataLastUpdate;
};

extern PromServer promServer;
#endif // ENABLE_PROMETHEUS_SERVER
