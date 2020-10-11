#pragma once
#include "Brewpi.h"
#include "TemperatureFormats.h"
#include "Ticks.h"

#if defined(ESP32)
#include "AsyncTCP.h"
#else
#include "ESPAsyncTCP.h"
#endif

#include "ESPAsyncWebServer.h"

/**
 * \brief A server for handling Prometheus metrics endpoint
 *
 * Implements the Prometheus [metrics
 * format](https://prometheus.io/docs/instrumenting/exposition_formats/).
 * Allows data gathering using an industry standard metrics system.  This opens
 * the possibility of more powerful reporting & alerting.
 *
 * Because enumerating and reading probes can take a relatively long time, this
 * class implements a cache so that subsequent data requests will use the
 * cached value.  This cache only applies to the probe values, not the entire
 * metrics response.  External code can use PromServer::invalidateCache() to
 * force cache updates.
 */
class PromServer {
public:
  void setup();
  static void onNotFound(AsyncWebServerRequest *request);
  static void metrics(AsyncWebServerRequest *request);
  static String templateProcessor(const String &var);
  static void invalidateCache();

private:
  /**
   * \brief ESPAsyncWebServer connection listener.
   *
   * Handles incoming requests to the Prometheus port and dispatches to handler
   * methods.
   * \see https://github.com/me-no-dev/ESPAsyncWebServer
   */
  AsyncWebServer server = AsyncWebServer(Config::Prometheus::port);
  static String probeCache;

  /**
   * \brief Template string for the metrics data.
   *
   * Placeholders values (surrounded by `%` chars) are replaced at runtime using
   * PromServer::templateProcessor().
   *
   * \see templateProcessor()
   */
  static const char metricsTemplate[];

  /**
   * \brief Template for an individual probe value
   *
   * Used to fill the PROBE_VALUES section of metricsTemplate for each
   * enumerated probe.
   *
   * \see probeValues()
   */
  static const char probeTemplate[];

  static String formatProbeTemp(const temperature temp);
  static String probeValues();

  /**
   * \brief How long to cache probe readings
   */
  static constexpr auto cacheTime = 300;

  /**
   * \brief Tick count of the last time the probe cache was built.
   *
   * Used to track the expiration of the probe cache.
   */
  static ticks_seconds_t dataLastUpdate;
};

extern PromServer promServer;
