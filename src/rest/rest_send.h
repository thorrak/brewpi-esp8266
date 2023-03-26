#pragma once


#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Ticker.h>
#include <ArduinoJson.hpp>


#define FULL_CONFIG_PUSH_DELAY      (5 * 60)    // 5 minute delay on pushing a "full config" to the endpoint
#define LCD_PUSH_DELAY              (25)        // 25 second delay on pushing LCD data to the endpoint


enum class sendResult {
    success,
    failure,
    retry
};

class restHandler
{
public:

    // Timers and semaphores
    Ticker fullConfigTicker;
    Ticker lcdTicker;

    bool send_full_config_ticker;
    bool send_lcd_ticker;


    restHandler();
    void init();

    bool send_bluetooth_crash_report();
    bool send_full_config();
    sendResult send_json_str(String &payload, const char *url);
    void get_useragent(char *ua, size_t size);

    // Everything below this MAY no longer be in use. Need to check. 
    void process();


    bool send_data();



    bool send_lock = false;

    HTTPClient http;

private:
    void get_url(char *url, size_t size, const char *path);
    WiFiClient client;
    WiFiClientSecure secureClient;
};


extern restHandler rest_handler;


