#ifdef HAS_BLUETOOTH
#include <Arduino.h>
#include "Tilt.h"
#include "TemperatureFormats.h"


void tilt::update(TiltColor color_index, uint16_t temp, uint16_t grav, uint8_t i_tx_pwr, int16_t rssi)
{
    if (temp == 999) { // If the temp is 999, the SG actually represents the firmware version of the Tilt.
        // version_code = grav;
        return; // This also has the (desired) side effect of not logging the 999 "temperature" and 1.00x "gravity"
    } else if (grav >= 5000) {  // If we received a gravity over 5000 then this Tilt is high resolution (Tilt Pro)
        tilt_pro = true;
    } else if (grav < 5000) {
        tilt_pro = false;
    }

    if (i_tx_pwr==197)
        m_has_sent_197 = true;
    else {
        if (m_has_sent_197)
            receives_battery = true;
        if (receives_battery) 
            battery_weeks = i_tx_pwr;
    }

    rawTemp=temp;
    rawGravity=grav;
    m_lastUpdate = millis();
    hasData = true;
    color = color_index;
    return;
}

uint16_t tilt::getTemp()
{
    return rawTemp;
}

uint16_t tilt::getGravity()
{
    return rawGravity;
}

bool tilt::isConnected()
{
    return millis() <= (m_lastUpdate + TILT_CONNECTED_TIMEOUT) && hasData;
}

TiltColor tilt::uuid_to_color_no(std::string uuid)
{

    if (uuid == TILT_COLOR_RED_UUID)
        return TiltColor::TILT_COLOR_RED;
    else if (uuid == TILT_COLOR_GREEN_UUID)
        return TiltColor::TILT_COLOR_GREEN;
    else if (uuid == TILT_COLOR_BLACK_UUID)
        return TiltColor::TILT_COLOR_BLACK;
    else if (uuid == TILT_COLOR_PURPLE_UUID)
        return TiltColor::TILT_COLOR_PURPLE;
    else if (uuid == TILT_COLOR_ORANGE_UUID)
        return TiltColor::TILT_COLOR_ORANGE;
    else if (uuid == TILT_COLOR_BLUE_UUID)
        return TiltColor::TILT_COLOR_BLUE;
    else if (uuid == TILT_COLOR_YELLOW_UUID)
        return TiltColor::TILT_COLOR_YELLOW;
    else if (uuid == TILT_COLOR_PINK_UUID)
        return TiltColor::TILT_COLOR_PINK;
    return TiltColor::TILT_COLOR_NONE;  // Should never get here
}

long_temperature tilt::getTempFixedPoint() {
	long_temperature fracPart = 0;

    // Tilts always report in Fahrenheit - convert to celsius and then split
    double conv_temp = (rawTemp / (tilt_pro ? 10 : 1) - 32) * 5 / 9;

    // Split into an integer and a fractional part
    double intPartRaw;
    double fracPartRaw = modf(conv_temp, &intPartRaw);

    // Int part is good to go - just recast it and scale it up
	long_temperature intPart = static_cast<int>(intPartRaw) * TEMP_FIXED_POINT_SCALE;

    // Fractional part needs to be scaled up to max precision (0.1deg F, using 0.01deg C instead)
    fracPart = static_cast<int>(trunc(fracPartRaw * 100));
	fracPart = fracPart << TEMP_FIXED_POINT_BITS; // bits for fraction part

    // Since we have two decimals
	fracPart = (fracPart + 5) / 10;
	fracPart = (fracPart + 5) / 10;

	long_temperature combinedTemp = (intPart) + fracPart + C_OFFSET;
	return combinedTemp;
}

#endif
