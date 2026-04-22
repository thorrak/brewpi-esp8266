/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
 * Copyright 2013 Matthew McGowan.
 *
 * This file is part of BrewPi.
 *
 * BrewPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BrewPi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BrewPi.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Brewpi.h"
#include "OneWireTempSensor.h"
#include "TemperatureFormats.h"
#include "OneWireScanner.h"


bool OneWireTempSensor::init() {
    // Let the scanner know the sensor exists and is interesting. Discovery and
    // first-read population happen on the worker task's cycle. Returning
    // isConnected() here matches the old contract; callers gate on it but the
    // cache will warm up within ~2s regardless.
    return isConnected();
}


bool OneWireTempSensor::isConnected() {
    onewire_device_record* rec = ow_scanner.get_by_bytes(m_sensor_address);
    return rec && rec->isConnected();
}


temperature OneWireTempSensor::read() {
    onewire_device_record* rec = ow_scanner.get_by_bytes(m_sensor_address);
    if (!rec || !rec->isConnected()) {
        return TEMP_SENSOR_DISCONNECTED;
    }

    // rec->getTempFixedPoint() returns the reading in long_temperature
    // precision with C_OFFSET already applied.
    long_temperature t = rec->getTempFixedPoint()
        + (((long_temperature)m_calibration_offset)
           << (TEMP_FIXED_POINT_BITS - TEMP_CALIBRATION_OFFSET_PRECISION));

    return constrainTemp(t, MIN_TEMP, MAX_TEMP);
}
