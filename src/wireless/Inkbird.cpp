#ifdef HAS_BLUETOOTH
#include <Arduino.h>
#include "Inkbird.h"
#include "TemperatureFormats.h"


void inkbird::update(int16_t temp, uint16_t hum, uint8_t bat, int16_t rssi)
{
    rawTemp=temp;
    rawHumidity=hum;
    battery=bat;
    m_lastUpdate = esp_timer_get_time();
    hasData = true;
    return;
}

int16_t inkbird::getTemp()
{
    return rawTemp;
}

uint16_t inkbird::getHumidity()
{
    return rawHumidity;
}

uint8_t inkbird::getBattery()
{
    return battery;
}

bool inkbird::isConnected()
{
    return esp_timer_get_time() <= (m_lastUpdate + INKBIRD_CONNECTED_TIMEOUT) && hasData;
}

long_temperature inkbird::getTempFixedPoint() {
	long_temperature intPart = 0;
	long_temperature fracPart = 0;
	
	intPart = (int) rawTemp/100*TEMP_FIXED_POINT_SCALE;

	fracPart = rawTemp % 100;
	fracPart = fracPart << TEMP_FIXED_POINT_BITS; // bits for fraction part

    // Since we have two decimals
	fracPart = (fracPart + 5) / 10;
	fracPart = (fracPart + 5) / 10;

	long_temperature combinedTemp = (intPart) + fracPart + C_OFFSET;
	return combinedTemp;
}

#endif
