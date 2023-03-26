//
// Created by Lee Bussy on 12/31/20
//

#include "uptime.h"

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
    if ((int)(millis() - uptimeNow) > refresh)
    {
        setValues();
    }
    // Reset timer for another period to avoid a really unlikely situation
    // where the timer expires in between grabbing two parts
    uptimeNow = millis();
}

void setValues()
{
    // Call this only by getNow()
    // Using refr = true forces recalculation
    uptimeNow = millis();
    days = uptimeDays(true);
    hours = uptimeHours(true);
    minutes = uptimeMinutes(true);
    seconds = uptimeSeconds(true);
    mills = uptimeMillis(true);
}

const int uptimeDays(bool refr)
{
    getNow(); // Make sure we are current
    if (refr)
    {
        // Calculate full days from uptime
        days = (int)floor(uptimeNow / DAY_MILLIS);
    }
    return days;
}

const int uptimeHours(bool refr)
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

const int uptimeMinutes(bool refr)
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

const int uptimeSeconds(bool refr)
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

const int uptimeMillis(bool refr)
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
