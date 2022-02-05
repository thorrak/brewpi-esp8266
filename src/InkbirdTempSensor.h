#pragma once
#ifdef HAS_BLUETOOTH
#include "Brewpi.h"
#include "TempSensor.h"
#include "FastDigitalPin.h"
#include "Ticks.h"
#include "DallasTemperature.h" // for DeviceAddress
#include <NimBLEDevice.h>

#include "wireless/Inkbird.h"


/**
 * \brief An Inkbird bluetooth temperature sensor
 *
 * \ingroup hardware
 */
class InkbirdTempSensor : public BasicTempSensor {
public:
    /**
     * \brief Constructs a new Inkbird temp sensor.
   *
     * /param address    The MAC address for this sensor.
     * /param calibration    A temperature value that is added to all readings. This can be used to calibrate the sensor.
     */
    InkbirdTempSensor(NimBLEAddress address, fixed4_4 calibrationOffset) {
        btAddress = address;

        this->calibrationOffset = calibrationOffset;
        ib = nullptr;
    };

    ~InkbirdTempSensor();

  /**
   * \brief Check if sensor device is connected
   */
    bool isConnected(){
        if (ib && ib->isConnected())
            return true;
        return false;
    }

    bool init();
    temperature read();

    NimBLEAddress btAddress;
    inkbird *ib;

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