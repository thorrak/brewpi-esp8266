#ifndef _RESETREASONS_H
#define _RESETREASONS_H

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

#endif // _RESETREASONS_H
