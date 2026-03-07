#pragma once
#include <ArduinoJson.h>
#include <stdint.h>

/**
 * \file ESP_BP_WiFi.h
 *
 * \defgroup wifi WiFi Configuration & Management
 * \brief WiFi configuration functions
 *
 * Various helper functions for interacting with the WiFi configuration.
 *
 * \addtogroup wifi
 * @{
 */

/**
 * \brief Initialize the WiFi client
 *
 * If WiFi is enabled, this sets it up. Otherwise, it disconnects the radio.
 */
void initialize_wifi();

/**
 * \brief Display the WiFi splash screen & trigger reconnection callback.
 */
void display_connect_info_and_create_callback();

/**
 * \brief Handle incoming WiFi client connections.
 *
 * This also handles WiFi network reconnects if the network was disconnected.
 */
void wifi_connect_clients();

/**
 * \brief Initialize the telnet server
 */
void initWifiServer();

/**
 * \brief Get current WiFi connection information, in JsonDocument format
 *
 * \param doc - JsonDocument to populate.
 */
void wifi_connection_info(JsonDocument& doc);

extern int telnet_server_fd;
extern int telnet_client_fd;

/**
 * \brief Reset (reinitialize) the mDNS responder with the current hostname.
 */
void mdns_reset();

// -----------------------------------------------------------------------
// ESP-IDF WiFi utility functions
//
// Thin wrappers around ESP-IDF calls that replace direct Arduino WiFi
// object usage.  Safe to call even when WiFi is not initialised (they
// return sensible defaults).  All functions assume single-threaded
// main-loop usage (static buffers are not guarded).
// -----------------------------------------------------------------------

/** \brief Return true if the STA interface has a valid IP address. */
bool bp_wifi_is_connected();

/** \brief Return the STA IP address as a dotted-decimal C string. */
const char* bp_wifi_get_ip_str();

/** \brief Return the STA IP address as a raw uint32_t. */
uint32_t bp_wifi_get_ip_addr();

/** \brief Disconnect from the AP. If erase_credentials is true, clear stored config. */
void bp_wifi_disconnect(bool erase_credentials);

/** \brief Return the SSID of the currently associated AP (or empty string). */
const char* bp_wifi_get_ssid();

/** \brief Return the RSSI of the currently associated AP (or 0). */
int8_t bp_wifi_get_rssi();

/** \brief Return the hostname configured on the STA interface. */
const char* bp_wifi_get_hostname();

/** \brief Trigger a reconnect using the stored STA config. */
void bp_wifi_reconnect();

/** \brief Fully shut down the WiFi subsystem (disconnect + stop + deinit). */
void bp_wifi_off();


/** @} */
