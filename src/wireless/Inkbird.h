#ifndef BREWPI_INKBIRD_H
#define BREWPI_INKBIRD_H

#ifdef HAS_BLUETOOTH


#include "Brewpi.h"
#include "TempSensor.h"
#include "FastDigitalPin.h"
// #include "Ticks.h"
#include <NimBLEDevice.h>
#include "TemperatureFormats.h"


#define INKBIRD_CONNECTED_TIMEOUT       (30 * 1000 * 1000)     // Time before the sensor is considered "disconnected" (in microseconds)

class inkbird
{
public:
    inkbird(const NimBLEAddress devAddress, int16_t temp, uint16_t hum, uint8_t bat, int16_t rssi)
    :deviceAddress(devAddress), m_lastUpdate(esp_timer_get_time()), rawTemp(temp), rawHumidity(hum), battery(bat), hasData(true) {};

    inkbird(const NimBLEAddress devAddress)
    :deviceAddress(devAddress), m_lastUpdate(0), rawTemp(0), rawHumidity(0), battery(0), hasData(false) {};

    void update(int16_t temp, uint16_t hum, uint8_t bat, int16_t rssi);
    int16_t getTemp();
    uint16_t getHumidity();
    uint8_t getBattery();
    long_temperature getTempFixedPoint();

    bool isConnected();

    NimBLEAddress deviceAddress;

    uint64_t m_lastUpdate; // Keep track of when we last updated to detect disconnection

    bool operator == (const inkbird& dev) const { return deviceAddress == dev.deviceAddress; }
    bool operator != (const inkbird& dev) const { return !operator==(dev); }

private:
    int16_t rawTemp;
    uint16_t rawHumidity;
    uint8_t battery;
    bool hasData;

};

#endif
#endif //BREWPI_INKBIRD_H
