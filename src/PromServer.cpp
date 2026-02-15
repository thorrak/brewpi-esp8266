#ifdef ENABLE_PROMETHEUS_SERVER

#include "Config.h"

#include "DeviceManager.h"
#include "DeviceNameManager.h"
#include "PromServer.h"
#include "TempControl.h"
#include "TemperatureFormats.h"
#include "Ticks.h"
#include <string>
#include <cstring>

const char PromServer::metricsTemplate[] PROGMEM =
    R"PROM(# HELP brewpi_uptime_seconds Number of seconds since the last hardware reset
# TYPE brewpi_uptime_seconds counter
brewpi_uptime_seconds %UPTIME%

# HELP brewpi_state Status of a controller actor
# TYPE brewpi_state gauge
brewpi_state{actor="cooler"} %COOLER_STATUS%
brewpi_state{actor="heater"} %HEATER_STATUS%

# HELP brewpi_target Target temperature
# TYPE brewpi_target gauge
brewpi_target{probe="beer"} %BEER_TARGET%
brewpi_target{probe="fridge"} %FRIDGE_TARGET%

# HELP brewpi_temperature Temperature
# TYPE brewpi_temperature gauge
brewpi_temperature{probe="beer"} %BEER_TEMP%
brewpi_temperature{probe="fridge"} %FRIDGE_TEMP%
brewpi_temperature{probe="room"} %ROOM_TEMP%

%PROBE_VALUES%
)PROM";

const char PromServer::probeTemplate[] PROGMEM =
    R"PROM(brewpi_temperature{probe="%s"} %s
)PROM";

std::string PromServer::probeCache;

ticks_seconds_t PromServer::dataLastUpdate = 0 - PromServer::cacheTime;


// Replace %VAR% placeholders in the template string
static void replaceVar(std::string& str, const char* var, const char* value) {
    char placeholder[64];
    snprintf(placeholder, sizeof(placeholder), "%%%s%%", var);
    size_t pos = str.find(placeholder);
    if (pos != std::string::npos) {
        str.replace(pos, strlen(placeholder), value);
    }
}

std::string PromServer::processTemplate() {
    std::string result(metricsTemplate);

    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)ticks.seconds());
    replaceVar(result, "UPTIME", buf);

    replaceVar(result, "COOLER_STATUS", tempControl.stateIsCooling() ? "1" : "0");
    replaceVar(result, "HEATER_STATUS", tempControl.stateIsHeating() ? "1" : "0");

    replaceVar(result, "BEER_TEMP", formatProbeTemp(tempControl.getBeerTemp()).c_str());
    replaceVar(result, "BEER_TARGET", formatProbeTemp(tempControl.getBeerSetting()).c_str());
    replaceVar(result, "FRIDGE_TEMP", formatProbeTemp(tempControl.getFridgeTemp()).c_str());
    replaceVar(result, "FRIDGE_TARGET", formatProbeTemp(tempControl.getFridgeSetting()).c_str());
    replaceVar(result, "ROOM_TEMP", formatProbeTemp(tempControl.getRoomTemp()).c_str());

    replaceVar(result, "PROBE_VALUES", probeValues().c_str());

    return result;
}


esp_err_t PromServer::metricsHandler(httpd_req_t *req) {
    std::string response = processTemplate();
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, response.c_str(), response.length());
}


void PromServer::setup() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = Config::Prometheus::port;
    config.max_uri_handlers = 4;
    config.stack_size = 8192;

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        return;
    }

    const httpd_uri_t uri_metrics = {
        .uri = "/metrics",
        .method = HTTP_GET,
        .handler = metricsHandler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server, &uri_metrics);
}


std::string PromServer::formatProbeTemp(const temperature temp) {
    char buf[10];
    tempToString(buf, temp, Config::TempFormat::fixedPointDecimals, Config::TempFormat::maxLength);

    if (strcmp(buf, "null") == 0)
        return "NaN";

    return std::string(buf);
}


void PromServer::invalidateCache() {
    if (Config::Prometheus::enable())
        PromServer::dataLastUpdate = 0 - PromServer::cacheTime;
}


std::string PromServer::probeValues() {
    if (ticks.timeSince(dataLastUpdate) > PromServer::cacheTime) {
        JsonDocument doc;
        deviceManager.rawDeviceValues(doc);

        JsonArray root = doc.as<JsonArray>();
        probeCache.clear();

        for (JsonVariant probe : root) {
            const char* devName = probe["device"].as<const char *>();
            std::string humanName = DeviceNameManager::getDeviceName(devName);

            char buffer[256];
            snprintf(buffer, sizeof(buffer), probeTemplate, humanName.c_str(), probe["value"].as<const char *>());

            probeCache += buffer;
        }

        dataLastUpdate = ticks.seconds();
    }

    return probeCache;
}

PromServer promServer;
#endif // ENABLE_PROMETHEUS_SERVER
