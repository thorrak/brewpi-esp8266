#pragma once
//#include <stdio.h>
//#include <stdlib.h>
//#include <string>

#include "Arduino.h"
#include "Brewpi.h"

#undef min
#undef max

#ifdef ESP8266_WiFi
#include <ESP8266WiFi.h>		//ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>	//Local WebServer used to serve the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#endif

#undef byte // Please just use uint8_t
#undef boolean // Please just use bool

#define WIRING 1

#define arraySize(x) (sizeof(x)/sizeof(x[0]))

#define PRINTF_PROGMEM "%S"             // on arduino, use the special format symbol

#define ONEWIRE_PIN

typedef uint32_t tcduration_t;
typedef uint32_t ticks_millis_t;
typedef uint32_t ticks_micros_t;
typedef uint32_t ticks_seconds_t;
typedef uint8_t ticks_seconds_tiny_t;

bool platform_init();

#define MAX_EEPROM_SIZE_LIMIT 1024

#ifdef ESP8266_WiFi
void saveConfigCallback();  //callback notifying us of the need to save config
bool isValidmDNSName(String mdns_name);  // Test to see if the mDNS name we were provided is valid during setup
void connectClients(); 

extern bool shouldSaveConfig;  // TODO - Determine if this should be extern
extern WiFiServer server;
extern WiFiClient serverClient;
#endif