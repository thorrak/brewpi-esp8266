#include <FS.h>  // Apparently this needs to be first
#include "Brewpi.h"
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#if defined(ESP8266)
#include <ESP8266WiFi.h>  // For printing the IP address
#include <time.h>
#elif defined(ESP32)
#include <WiFi.h> // For printing the IP address
#include <time.h>
#include "esp_sntp.h"
#endif


#include "ntp.h"

bool ntpTimeSynced = false;


// NTP Configuration
constexpr const char* NTP_SERVER1 = "pool.ntp.org";
constexpr const char* NTP_SERVER2 = "time.nist.gov";
constexpr long GMT_OFFSET_SEC = 0;  // Use UTC for logging
constexpr int DAYLIGHT_OFFSET_SEC = 0;

/**
 * \brief Initialize NTP time synchronization
 * \ingroup wifi
 */
void initNTP() {
    if (WiFi.status() != WL_CONNECTED) return;

#if defined(ESP32)
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);

    // Wait for time to be set (max 10 seconds)
    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 10) {
        delay(1000);
        retry++;
    }

    if (retry < 10) {
        ntpTimeSynced = true;
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("NTP time synchronized: %s UTC\n", timeStr);
    } else {
        Serial.println("Failed to synchronize NTP time");
    }
#elif defined(ESP8266)
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);

    // Wait for time to be set (max 10 seconds)
    time_t now = time(nullptr);
    int retry = 0;
    while (now < 1000000000 && retry < 10) {
        delay(1000);
        now = time(nullptr);
        retry++;
    }

    if (retry < 10) {
        ntpTimeSynced = true;
        struct tm* timeinfo = localtime(&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
        Serial.printf("NTP time synchronized: %s UTC\n", timeStr);
    } else {
        Serial.println("Failed to synchronize NTP time");
    }
#endif
}

/**
 * \brief Check if NTP time has been synchronized
 * \return true if time is synchronized
 */
bool isNtpSynced() {
    return ntpTimeSynced;
}

/**
 * \brief Get current time as formatted string
 * \param buffer - Buffer to store the formatted time
 * \param bufferSize - Size of the buffer
 * \return true if time was successfully formatted
 */
bool getFormattedTime(char* buffer, size_t bufferSize) {
    if (!ntpTimeSynced) {
        snprintf(buffer, bufferSize, "0");
        return false;
    }

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", timeinfo);
    return true;
}


