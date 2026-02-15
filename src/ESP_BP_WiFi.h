#pragma once
#include <ArduinoJson.h>

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

// This library always needs to get loaded, as we're going to need to interact with the radio regardless of whether
// we're using WiFi or not.
#include <WiFi.h> // For WiFi management (WiFiManager, WiFi.status(), etc.)


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



/** @} */
