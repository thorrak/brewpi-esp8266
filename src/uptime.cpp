//
// Created by Lee Bussy on 12/31/20
//

#include "uptime.h"
#include <esp_timer.h>
#include <cmath>

static int refresh = UPTIME_REFRESH * 1000;
static unsigned long uptimeNow;
static int days;
static int hours;
static int minutes;
static int seconds;
static int mills;

void getNow()
{
    // Set the uptime values if refresh time is expired
    if ((int)((unsigned long)(esp_timer_get_time() / 1000ULL) - uptimeNow) > refresh)
    {
        setValues();
    }
    // Reset timer for another period to avoid a really unlikely situation
    // where the timer expires in between grabbing two parts
    uptimeNow = (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void setValues()
{
    // Call this only by getNow()
    // Using refr = true forces recalculation
    uptimeNow = (unsigned long)(esp_timer_get_time() / 1000ULL);
    days = uptimeDays(true);
    hours = uptimeHours(true);
    minutes = uptimeMinutes(true);
    seconds = uptimeSeconds(true);
    mills = uptimeMillis(true);
}

int uptimeDays(bool refr)
{
    getNow(); // Make sure we are current
    if (refr)
    {
        // Calculate full days from uptime
        days = (int)floor(uptimeNow / DAY_MILLIS);
    }
    return days;
}

int uptimeHours(bool refr)
{
    getNow(); // Make sure we are current
    if (refr)
    {
        // Refresh values:
        // Subtract millis value for any full days via modulo
        // Calculate full hours from remainder
        hours = (int)floor((uptimeNow % DAY_MILLIS) / HOUR_MILLIS);
    }
    return hours;
}

int uptimeMinutes(bool refr)
{
    getNow(); // Make sure we are current
    if (refr)
    {
        // Refresh values:
        // Subtract millis value for any full hours via modulo
        // Calculate full minutes from remainder
        minutes = (int)floor((uptimeNow % HOUR_MILLIS) / MIN_MILLIS);
    }
    return minutes;
}

int uptimeSeconds(bool refr)
{
    getNow(); // Make sure we are current
    if (refr)
    {
        // Refresh values:
        // Subtract millis value for any full minutes via modulo
        // Calculate full seconds from remainder
        seconds = (int)floor((uptimeNow % MIN_MILLIS) / SEC_MILLIS);
    }
    return seconds;
}

int uptimeMillis(bool refr)
{
    getNow(); // Make sure we are current
    if (refr)
    {
        // Refresh values:
        // Subtract millis value for any full seconds via modulo
        // Return remainder millis
        mills = (int)floor((uptimeNow % SEC_MILLIS));
    }
    return mills;
}
