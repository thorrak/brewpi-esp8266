#pragma once
#ifdef HAS_BLUETOOTH
#include "Brewpi.h"
#include "TempSensor.h"
#include "FastDigitalPin.h"
#include "Ticks.h"
// #include "DallasTemperature.h" // for DeviceAddress
#include <NimBLEDevice.h>

#include "wireless/Tilt.h"


/**
 * \brief A Tilt bluetooth temperature sensor
 *
 * \ingroup hardware
 */
class TiltTempSensor : public BasicTempSensor {
public:
    /**
     * \brief Constructs a new Tilt temp sensor.
   *
     * /param address    The MAC address for this sensor.
     * /param calibration    A temperature value that is added to all readings. This can be used to calibrate the sensor.
     */
    TiltTempSensor(NimBLEAddress address, fixed4_4 calibrationOffset) {
        btAddress = address;

        this->calibrationOffset = calibrationOffset;
        th = nullptr;
    };

    ~TiltTempSensor();

  /**
   * \brief Check if sensor device is connected
   */
    bool isConnected(){
        if (th && th->isConnected())
            return true;
        return false;
    }

    bool init();
    temperature read();

    NimBLEAddress btAddress;
    tilt *th;

private:
  /**
   * \brief The sensor precision, in bits.
   */
//   constexpr static uint8_t sensorPrecision = 4;

    temperature readAndConstrainTemp();
    // uint8_t sensorAddress[6];

    fixed4_4 calibrationOffset; //!< Temperature offset needed for calibration

};
#endif