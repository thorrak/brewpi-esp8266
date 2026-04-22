/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
 * Copyright 2013 Matthew McGowan
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

#pragma once

#include "Brewpi.h"
#include "TempSensor.h"
#include "FastDigitalPin.h"
#include "Ticks.h"
#include <string.h>

typedef uint8_t DeviceAddress[8];

/**
 * \brief A OneWire attached temperature sensor.
 *
 * This class is a thin wrapper over the cached record maintained by
 * OneWireScanner. All bus I/O happens on the scanner's worker task; read()
 * just returns the cached value. Device discovery, conversion, and bus
 * recovery are all the scanner's responsibility.
 *
 * \ingroup hardware
 */
class OneWireTempSensor : public BasicTempSensor {
public:
  /**
   * \brief Constructs a new onewire temp sensor.
   *
   * @param address     OneWire ROM ID for this sensor.
   * @param calibration A temperature offset added to readings. Stored in
   *                    TEMP_CALIBRATION_OFFSET_PRECISION fractional bits.
   */
  OneWireTempSensor(const DeviceAddress address, fixed4_4 calibrationOffset)
    : m_calibration_offset(calibrationOffset) {
    memcpy(m_sensor_address, address, sizeof(DeviceAddress));
  }

  ~OneWireTempSensor() = default;

  bool isConnected() override;

  bool init() override;
  temperature read() override;

private:
  DeviceAddress m_sensor_address;
  fixed4_4 m_calibration_offset;
};
