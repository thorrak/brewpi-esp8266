#include "Config.h"

#include "DeviceManager.h"
#include "DeviceNameManager.h"
#include "PromServer.h"
#include "TempControl.h"
#include "TemperatureFormats.h"
#include "Ticks.h"

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

/**
 * \brief Storage for caching probe values
 */
String PromServer::probeCache = String("");

// Initialize this as a negative so that we immediately are in an expired
// state, otherwise you have to wait around before the first read actually
// happens.
ticks_seconds_t PromServer::dataLastUpdate = 0 - PromServer::cacheTime;

/**
 * \brief Set up the request endpoints
 *
 * Maps paths to handler functions and starts the server.
 */
void PromServer::setup() {
  server.on("/metrics", HTTP_GET, PromServer::metrics);

  server.onNotFound(PromServer::onNotFound);
  server.begin();
}

/**
 * \brief Return 404 for unknown requests
 * \param request - Request object
 */
void PromServer::onNotFound(AsyncWebServerRequest *request) { request->send(404); }

/**
 * \brief Handle the metrics request
 * \param request - Request object
 */
void PromServer::metrics(AsyncWebServerRequest *request) {
  request->send_P(200, "text/plain", metricsTemplate, PromServer::templateProcessor);
}

/**
 * \brief Supply template placeholder values
 *
 * Used by AsyncWebServer when handling the metrics response to supply values
 * for the placeholders used in the PromServer::metricsTemplate template
 * string.
 *
 * \param val - Placeholder string being replaced
 * \return Replacement value.  If a defined replacement for `val` isn't known,
 * an empty string is returned.
 */
String PromServer::templateProcessor(const String &var) {
  if (var == "UPTIME")
    return String(ticks.seconds());

  // Temp control state
  if (var == "COOLER_STATUS")
    return tempControl.stateIsCooling() ? "1" : "0";

  if (var == "HEATER_STATUS")
    return tempControl.stateIsHeating() ? "1" : "0";

  if (var == "BEER_TEMP")
    return formatProbeTemp(tempControl.getBeerTemp());

  if (var == "BEER_TARGET")
    return formatProbeTemp(tempControl.getBeerSetting());

  if (var == "FRIDGE_TEMP")
    return formatProbeTemp(tempControl.getFridgeTemp());

  if (var == "FRIDGE_TARGET")
    return formatProbeTemp(tempControl.getFridgeSetting());

  if (var == "ROOM_TEMP")
    return formatProbeTemp(tempControl.getRoomTemp());

  // Probe readings
  if (var == "PROBE_VALUES")
    return probeValues();

  return String();
}

/**
 * \brief Format a probe value into a String
 *
 * The Prometheus format wants `NaN` to be used to indicate a missing/invalid
 * data. tempToString() uses `null`.  This method patches the output value to
 * replace `null` with `NaN`
 *
 * \param temp - Temperature value to format
 * \return String representation of temperature
 */
String PromServer::formatProbeTemp(const temperature temp) {
  char buf[10];
  tempToString(buf, temp, Config::TempFormat::fixedPointDecimals, Config::TempFormat::maxLength);
  const String ret = String(buf);

  // Prometheus wants 'NaN' to for null data
  if (ret.equals("null"))
    return String("NaN");

  return ret;
}

/**
 * \brief Invalidate the probe values cache
 */
void PromServer::invalidateCache() {
  if (Config::Prometheus::enable())
    PromServer::dataLastUpdate = 0 - PromServer::cacheTime;
}

/**
 * \brief Provide metrics strings for all probes
 *
 * Uses PromServer::probeTemplate to produce metrics for every attached temp
 * probe, even those that aren't used in the control loop. If the device has a
 * human readable name registered with DeviceNameManager, it will be reported.
 *
 * Because reading all probes on the OneWire bus takes a while, this will cache
 * the values internally.
 *
 * \see PromServer::cacheTime
 * \return Formatted metrics string
 */
String PromServer::probeValues() {
  if (ticks.timeSince(dataLastUpdate) > PromServer::cacheTime) {
    DynamicJsonDocument doc(1024);
    deviceManager.rawDeviceValues(doc);

    JsonArray root = doc.as<JsonArray>();
    probeCache = String("");

    for (JsonVariant probe : root) {
      String devName(probe["device"].as<const char *>());
      String humanName = DeviceNameManager::getDeviceName(devName.c_str());

      char buffer[256];
      sprintf_P(buffer, probeTemplate, humanName.c_str(), probe["value"].as<const char *>());

      probeCache += buffer;
    }

    // Update the cache timestamp
    dataLastUpdate = ticks.seconds();
  }

  return probeCache;
}

PromServer promServer;
