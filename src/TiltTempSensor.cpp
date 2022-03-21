#ifdef HAS_BLUETOOTH
#include "Brewpi.h"
#include "TiltTempSensor.h"
#include "PiLink.h"  // For printBytes
#include "TemperatureFormats.h"

#include "wireless/BTScanner.h"

TiltTempSensor::~TiltTempSensor(){
    // delete sensor;
};

/**
 * \brief Initializes the temperature sensor.
 *
 * This method is called when the sensor is first created and also any time the
 * sensor reports it's disconnected.  If the result is TEMP_SENSOR_DISCONNECTED
 * then subsequent calls to read() will also return TEMP_SENSOR_DISCONNECTED.
 * Clients should attempt to re-initialize the sensor by calling init() again.
 */
bool TiltTempSensor::init() {
    if (th==NULL) {
        th = bt_scanner.get_or_create_tilt(btAddress);
        if (th==NULL) {
            logErrorString(ERROR_SRAM_SENSOR, btAddress.toString().c_str());
        }
    }
    return isConnected();
}


/**
 * \brief Read the value of the sensor
 *
 * @return TEMP_SENSOR_DISCONNECTED if sensor is not connected, constrained temp otherwise.
 * @see readAndConstrainTemp()
 */
temperature TiltTempSensor::read(){
    if (!isConnected())
        return TEMP_SENSOR_DISCONNECTED;

    return readAndConstrainTemp();
}


/**
 * \brief Reads the temperature.
 *
 * If successful, constrains the temp to the range of the temperature type
 * and updates lastRequestTime. On successful, leaves lastRequestTime alone
 * and returns TEMP_SENSOR_DISCONNECTED.
 */
temperature TiltTempSensor::readAndConstrainTemp()
{
    if(!isConnected()) {
        return TEMP_SENSOR_DISCONNECTED;
    }

    // double rawTemp = ib->getTemp() / 1000;
    // temperature temp = doubleToTemp(rawTemp);

    // const uint8_t shift = TEMP_FIXED_POINT_BITS - sensorPrecision; // difference in precision between DS18B20 format and temperature adt
    // temp = constrainTemp(temp+calibrationOffset+(C_OFFSET>>shift), ((int) MIN_TEMP)>>shift, ((int) MAX_TEMP)>>shift)<<shift;
    // TODO - Determine if I need to add/subtract C_OFFSET
    temperature temp = constrainTemp(th->getTempFixedPoint(), MIN_TEMP, MAX_TEMP);
    return temp;
}
#endif
