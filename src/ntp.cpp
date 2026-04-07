#include "Brewpi.h"

#include <esp_wifi.h>
#include <esp_sntp.h>
#include <time.h>
#include <thorlog.h>
#include <thorlog_espidf.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ntp.h"

static bool ntpTimeSynced = false;

// NTP Configuration
static constexpr const char* NTP_SERVER1 = "pool.ntp.org";
static constexpr const char* NTP_SERVER2 = "time.nist.gov";
static constexpr long GMT_OFFSET_SEC = 0;  // Use UTC for logging
static constexpr int DAYLIGHT_OFFSET_SEC = 0;

/**
 * \brief Initialize NTP time synchronization
 */
void initNTP() {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) return;  // Not connected

    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER1);
    esp_sntp_setservername(1, NTP_SERVER2);
    esp_sntp_init();

    // Wait for time to be set (max 10 seconds)
    struct tm timeinfo;
    int retry = 0;
    time_t now = 0;
    while (retry < 10) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year > (2020 - 1900)) {
            break;
        }
        retry++;
    }

    if (timeinfo.tm_year > (2020 - 1900)) {
        ntpTimeSynced = true;
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Log.info("NTP time synchronized: %s UTC\r\n", timeStr);
    } else {
        Log.warning("Failed to synchronize NTP time\r\n");
    }
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
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return true;
}
