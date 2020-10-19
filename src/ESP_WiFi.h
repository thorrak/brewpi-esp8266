//
// Created by John Beeler on 1/12/20.
//
#pragma once
#include <ArduinoJson.h>

/**
 * \file ESP_WiFi.h
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
// we're using WiFi or not. If neither ESP8266 or ESP32 is defined, (properly) do nothing (though this shouldn't
// actually compile for Arduino)
#if defined(ESP8266)
#include <ESP8266WiFi.h>  // For printing the IP address
#elif defined(ESP32)
#include <WiFi.h> // For printing the IP address
#endif



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

extern WiFiServer server;
extern WiFiClient serverClient;

/** @} */
