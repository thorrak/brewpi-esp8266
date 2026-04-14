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
#include "onewire_bus.h"
#include "ds18b20.h"

typedef uint8_t DeviceAddress[8];

/**
 * \brief A OneWire attached temperature sensor
 *
 * \ingroup hardware
 */
class OneWireTempSensor : public BasicTempSensor {
public:
  /**
   * \brief Constructs a new onewire temp sensor.
   *
   * /param bus	The onewire bus this sensor is on.
   * /param address	The onewire address for this sensor. If all bytes are 0 in the address, the first temp sensor
   *    on the bus is used.
   * /param calibration	A temperature value that is added to all readings. This can be used to calibrate the sensor.
   */
  OneWireTempSensor(onewire_bus_handle_t bus, DeviceAddress address, fixed4_4 calibrationOffset);

  ~OneWireTempSensor();

  /**
   * \brief Check if sensor device is connected
   */
  bool isConnected(){
    return m_connected;
  }

  bool init();
  temperature read();

  /**
   * \brief Invalidate this sensor's device handle after a bus reset.
   *
   * Deletes the ds18b20 device handle (which references the old bus),
   * updates the bus handle, and marks disconnected so init() re-enumerates.
   */
  void invalidateDevice(onewire_bus_handle_t newBus);

  /**
   * \brief Invalidate all OneWireTempSensor instances after a bus reset.
   */
  static void invalidateAllDevices(onewire_bus_handle_t newBus);

  /**
   * \brief Check if a bus-level recovery (teardown + recreate) is needed.
   *
   * Returns true when all OneWire init attempts have failed for longer
   * than BUS_RECOVERY_TIMEOUT_MS and the cooldown period has elapsed.
   */
  static bool needsBusRecovery();

  /**
   * \brief Notify that a bus reset was performed, resetting failure tracking.
   */
  static void notifyBusReset();

private:
  /**
   * \brief The sensor precision, in bits.
   */
  constexpr static uint8_t sensorPrecision = 4;

  void setConnected(bool connected);
  bool requestConversion();

  /**
   * \brief Wait for sensor to sample the environment
   */
  void waitForConversion()
  {
    wait.millis(750);
  }

  temperature readTempWithRetries(uint8_t attempts);
  temperature readAndConstrainTemp();

  onewire_bus_handle_t m_bus;
  ds18b20_device_handle_t m_sensor;
  DeviceAddress m_sensor_address;

  fixed4_4 m_calibration_offset;
  bool m_connected;
  uint8_t m_conversion_failures; //!< Consecutive conversion request failures

  // --- Static instance tracking for bus recovery ---
  static constexpr uint8_t MAX_INSTANCES = 8;
  static OneWireTempSensor* s_instances[MAX_INSTANCES];
  static uint8_t s_instance_count;

  // --- Bus failure tracking ---
  static constexpr uint32_t BUS_RECOVERY_TIMEOUT_MS = 30000;   // 30s of failed inits before bus reset
  static constexpr uint32_t BUS_RECOVERY_COOLDOWN_MS = 120000;  // Don't reset bus more than once per 2 min
  static bool s_bus_failing;
  static uint32_t s_first_bus_failure_time;
  static uint32_t s_last_bus_reset_time;
};
