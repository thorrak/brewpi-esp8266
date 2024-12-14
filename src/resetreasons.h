#ifndef _RESETREASONS_H
#define _RESETREASONS_H

#if defined ESP8266
const char *resetReason[7] = {
    "REASON_DEFAULT_RST",      // = 0, /* normal startup by power on */
    "REASON_WDT_RST",          // = 1, /* hardware watch dog reset */
    "REASON_EXCEPTION_RST",    // = 2, /* exception reset, GPIO status won’t change */
    "REASON_SOFT_WDT_RST",     // = 3, /* software watch dog reset, GPIO status won’t change */
    "REASON_SOFT_RESTART",     // = 4, /* software restart ,system_restart , GPIO status won’t change */
    "REASON_DEEP_SLEEP_AWAKE", // = 5, /* wake up from deep-sleep */
    "REASON_EXT_SYS_RST"       //  = 6 /* external system reset */
};

const char *resetDescription[7] = {
    "Normal startup by power on",
    "Hardware watch dog reset",
    "Exception reset, GPIO status won’t change",
    "Software watch dog reset, GPIO status won’t change",
    "Software restart, system_restart, GPIO status won’t change",
    "Wake up from deep-sleep",
    "External system reset"};

#else

const char *resetReason[11] = {
    "ESP_RST_UNKNOWN",    //!< Reset reason can not be determined
    "ESP_RST_POWERON",    //!< Reset due to power-on event
    "ESP_RST_EXT",        //!< Reset by external pin (not applicable for ESP32)
    "ESP_RST_SW",         //!< Software reset via esp_restart
    "ESP_RST_PANIC",      //!< Software reset due to exception/panic
    "ESP_RST_INT_WDT",    //!< Reset (software or hardware) due to interrupt watchdog
    "ESP_RST_TASK_WDT",   //!< Reset due to task watchdog
    "ESP_RST_WDT",        //!< Reset due to other watchdogs
    "ESP_RST_DEEPSLEEP",  //!< Reset after exiting deep sleep mode
    "ESP_RST_BROWNOUT",   //!< Brownout reset (software or hardware)
    "ESP_RST_SDIO"        //!< Reset over SDIO
};

const char *resetDescription[11] = {
    "Reset reason can not be determined",
    "Reset due to power-on event",
    "Reset by external pin (not applicable for ESP32)",
    "Software reset via esp_restart",
    "Software reset due to exception/panic",
    "Reset (software or hardware) due to interrupt watchdog",
    "Reset due to task watchdog",
    "Reset due to other watchdogs",
    "Reset after exiting deep sleep mode",
    "Brownout reset (software or hardware)",
    "Reset over SDIO"
};

#endif

#endif // _RESETREASONS_H
