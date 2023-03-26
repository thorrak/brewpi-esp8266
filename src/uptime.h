//
// Created by Lee Bussy on 12/31/20
//

#ifndef _UPTIME_H
#define _UPTIME_H

#include <Arduino.h>

#define UPTIME_REFRESH 1

#define DAY_MILLIS 86400000
#define HOUR_MILLIS 3600000
#define MIN_MILLIS 60000
#define SEC_MILLIS 1000

void getNow();
void setValues();
const int uptimeDays(bool refr = false);
const int uptimeHours(bool refr = false);
const int uptimeMinutes(bool refr = false);
const int uptimeSeconds(bool refr = false);
const int uptimeMillis(bool refr = false);

#endif // _UPTIME_H
