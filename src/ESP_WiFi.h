//
// Created by John Beeler on 1/12/20.
//
#pragma once

/**
 * \file ESP_WiFi.h
 *
 * \brief WiFi configuration
 */

// This library always needs to get loaded, as we're going to need to interact with the radio regardless of whether
// we're using WiFi or not. If neither ESP8266 or ESP32 is defined, (properly) do nothing (though this shouldn't
// actually compile for Arduino)
#if defined(ESP8266)
#include <ESP8266WiFi.h>  // For printing the IP address
#elif defined(ESP32)
#include <WiFi.h> // For printing the IP address
#endif



void initialize_wifi();  // If WiFi is enabled, this sets it up. Otherwise, it disconnects the radio.
void display_connect_info_and_create_callback(); // Display the WiFi splash screen & trigger reconnection callback
void wifi_connect_clients();
void initWifiServer();

extern WiFiServer server;
extern WiFiClient serverClient;
