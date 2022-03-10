#ifndef BREWPI_TILT_H
#define BREWPI_TILT_H

#ifdef HAS_BLUETOOTH


#include "Brewpi.h"
#include "TempSensor.h"
#include "FastDigitalPin.h"
// #include "Ticks.h"
#include <NimBLEDevice.h>
#include "TemperatureFormats.h"


#define TILT_COLOR_RED_UUID    "a495bb10c5b14b44b5121370f02d74de"
#define TILT_COLOR_GREEN_UUID  "a495bb20c5b14b44b5121370f02d74de"
#define TILT_COLOR_BLACK_UUID  "a495bb30c5b14b44b5121370f02d74de"
#define TILT_COLOR_PURPLE_UUID "a495bb40c5b14b44b5121370f02d74de"
#define TILT_COLOR_ORANGE_UUID "a495bb50c5b14b44b5121370f02d74de"
#define TILT_COLOR_BLUE_UUID   "a495bb60c5b14b44b5121370f02d74de"
#define TILT_COLOR_YELLOW_UUID "a495bb70c5b14b44b5121370f02d74de"
#define TILT_COLOR_PINK_UUID   "a495bb80c5b14b44b5121370f02d74de"


/**
 * \brief Tilt Hydrometer color index
 */
enum TiltColor {
    TILT_COLOR_NONE,
    TILT_COLOR_RED,
    TILT_COLOR_GREEN,
    TILT_COLOR_BLACK,
    TILT_COLOR_PURPLE,
    TILT_COLOR_ORANGE,
    TILT_COLOR_BLUE,
    TILT_COLOR_YELLOW,
    TILT_COLOR_PINK
};


/**
 * \brief Tilt Color Names
 * \see TiltColor
 */
namespace TiltColorNames {
    constexpr auto none = "None";
    constexpr auto red = "Red";
    constexpr auto green = "Green";
    constexpr auto black = "Black";
    constexpr auto purple = "Purple";
    constexpr auto orange = "Orange";
    constexpr auto blue = "Blue";
    constexpr auto yellow = "Yellow";
    constexpr auto pink = "Pink";
};


#define TILT_CONNECTED_TIMEOUT       (30 * 1000)     // Time before the sensor is considered "disconnected" (in ms)

class tilt
{
public:
    // tilt(const NimBLEAddress devAddress, TiltColor color_index, uint16_t temp, uint16_t grav, int16_t rssi)
    // :deviceAddress(devAddress), color(color_index), m_lastUpdate(millis()), battery_weeks(0), 
    // receives_battery(false), tilt_pro(false), rawTemp(temp), rawGravity(grav), hasData(true) {};

    // tilt(const NimBLEAddress devAddress, TiltColor color_index)
    // :deviceAddress(devAddress), color(color_index), m_lastUpdate(0), battery_weeks(0), 
    // receives_battery(false), tilt_pro(false), rawTemp(0), rawGravity(0), hasData(false) {};

    tilt(const NimBLEAddress devAddress)
    :deviceAddress(devAddress), color(TiltColor::TILT_COLOR_NONE), m_lastUpdate(0), battery_weeks(0), 
    receives_battery(false), tilt_pro(false), rawTemp(0), rawGravity(0), hasData(false) {};


    void update(TiltColor color_index, uint16_t temp, uint16_t grav, uint8_t i_tx_pwr, int16_t rssi);

    std::string get_color_string();


    uint16_t getTemp();
    uint16_t getGravity();
    long_temperature getTempFixedPoint();
    bool isConnected();
    static TiltColor uuid_to_color_no(std::string uuid);

    NimBLEAddress deviceAddress;
    TiltColor color;
    uint32_t m_lastUpdate; // Keep track of when we last updated to detect disconnection
    uint8_t battery_weeks;  // Weeks since the last battery change (if receives_battery is true)
    bool receives_battery;  // Tracks if this tilt sends battery life
    bool tilt_pro;  // Tracks if this tilt is "high resolution" or not (ie. is a Tilt Pro)

    bool operator == (const tilt& dev) const { return (deviceAddress == dev.deviceAddress && color == dev.color); }
    bool operator != (const tilt& dev) const { return !operator==(dev); }

private:
    uint16_t rawTemp;
    uint16_t rawGravity;
    bool hasData;
    bool m_has_sent_197;        // Used to determine if the tilt sends battery life (a 197 tx_pwr followed by a non-197 tx_pwr)

};

#endif
#endif //BREWPI_TILT_H
