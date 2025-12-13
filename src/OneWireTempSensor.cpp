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
#ifndef ESP8266
#include "Brewpi.h"
#include "OneWireTempSensor.h"
#include "EspDS18B20.h"
#include "PiLink.h"
#include "Ticks.h"
#include "TemperatureFormats.h"
#include "NumberFormats.h"
#include "onewire_device.h"
#include <string.h>

OneWireTempSensor::OneWireTempSensor(onewire_bus_handle_t bus, DeviceAddress address, fixed4_4 calibrationOffset)
  : m_bus(bus), m_sensor(NULL), m_connected(true), m_calibration_offset(calibrationOffset) {
  memcpy(m_sensor_address, address, sizeof(DeviceAddress));
}

OneWireTempSensor::~OneWireTempSensor(){
  if (m_sensor) {
    ds18b20_del_device(m_sensor);
  }
}

/**
 * \brief Initializes the temperature sensor.
 *
 * This method is called when the sensor is first created and also any time the
 * sensor reports it's disconnected.  If the result is TEMP_SENSOR_DISCONNECTED
 * then subsequent calls to read() will also return TEMP_SENSOR_DISCONNECTED.
 * Clients should attempt to re-initialize the sensor by calling init() again.
 *
 * Retries up to 3 times to improve resilience against transient connection issues.
 */
bool OneWireTempSensor::init() {
  char addressString[17];
  printBytes(m_sensor_address, 8, addressString);

  bool success = false;

  if (m_sensor == NULL) {
    logDebug("init onewire sensor - creating device");

    // Create device from address
    uint64_t address64 = bytesToAddress(m_sensor_address);

    // Check if address is all zeros (scan for first device)
    bool is_null_address = true;
    for (int i = 0; i < 8; i++) {
      if (m_sensor_address[i] != 0) {
        is_null_address = false;
        break;
      }
    }

    ds18b20_config_t ds_cfg = {};

    if (is_null_address) {
      // No specific address, use first device on bus
      esp_err_t ret = ds18b20_new_device_from_bus(m_bus, &ds_cfg, &m_sensor);
      if (ret != ESP_OK) {
        logErrorString(ERROR_SRAM_SENSOR, addressString);
        setConnected(false);
        return false;
      }
    } else {
      // Enumerate devices to find the matching address
      onewire_device_iter_handle_t iter = NULL;
      onewire_device_t next_device;

      if (onewire_new_device_iter(m_bus, &iter) != ESP_OK) {
        logErrorString(ERROR_SRAM_SENSOR, addressString);
        setConnected(false);
        return false;
      }

      bool found = false;
      while (onewire_device_iter_get_next(iter, &next_device) == ESP_OK) {
        if (next_device.address == address64) {
          if (ds18b20_new_device_from_enumeration(&next_device, &ds_cfg, &m_sensor) == ESP_OK) {
            found = true;
            break;
          }
        }
      }
      onewire_del_device_iter(iter);

      if (!found) {
        logErrorString(ERROR_SRAM_SENSOR, addressString);
        setConnected(false);
        return false;
      }
    }

    // Set resolution to 12-bit (750ms conversion time)
    ds18b20_set_resolution(m_sensor, DS18B20_RESOLUTION_12B);
  }

  logDebug("init onewire sensor");

  if (m_sensor && requestConversion()) {
    logDebug("init onewire sensor - wait for conversion");
    waitForConversion();
    temperature temp = readAndConstrainTemp();
    DEBUG_ONLY(logInfoIntStringTemp(INFO_TEMP_SENSOR_INITIALIZED, oneWirePin, addressString, temp));
    success = temp != TEMP_SENSOR_DISCONNECTED && requestConversion();
  }

  setConnected(success);
  logDebug("init onewire sensor complete %d", success);
  return success;
}


/**
 * \brief Request sensor measurement
 *
 * Sends a request to the OneWire bus for the configured device address to
 * begin the process of taking a measurement.  The length of time required for
 * a OneWire device to sample the temperature depend on the requested precision
 * and powered vs. parasite powered.
 *
 * Retries up to `attempts` times if the conversion request fails.
 *
 * @see waitForConversion()
 */
bool OneWireTempSensor::requestConversion() {
  if (!m_sensor) {
    return false;
  }

  const uint8_t attempts = 10;
  for (uint8_t i = 0; i < attempts; i++) {
    if (ds18b20_trigger_temperature_conversion(m_sensor) == ESP_OK) {
      setConnected(true);
      return true;
    }
    delay(50);
  }
  return false;
}

/**
 * \brief Set sensor connection status
 */
void OneWireTempSensor::setConnected(bool connected) {
  if (m_connected == connected)
    return;

  char addressString[17];
  printBytes(m_sensor_address, 8, addressString);
  m_connected = connected;
  if (connected) {
    logInfoIntString(INFO_TEMP_SENSOR_CONNECTED, oneWirePin, addressString);
  } else {
    logWarningIntString(WARNING_TEMP_SENSOR_DISCONNECTED, oneWirePin, addressString);
  }
}

/**
 * \brief Read the value of the sensor
 *
 * @return TEMP_SENSOR_DISCONNECTED if sensor is not connected, constrained temp otherwise.
 * @see readAndConstrainTemp()
 */
temperature OneWireTempSensor::read() {
  if (!m_connected)
    return TEMP_SENSOR_DISCONNECTED;

  temperature temp = readAndConstrainTemp();
  requestConversion();
  return temp;
}


/**
 * \brief Reads the temperature with retries.
 *
 * Attempts to read the temperature up to a specified number of times.
 * If successful, returns the temperature. If unsuccessful, returns DEVICE_DISCONNECTED_RAW.
 */
temperature OneWireTempSensor::readTempWithRetries(uint8_t attempts) {
  int16_t temp;
  for (uint8_t i = 0; i < attempts; i++) {
    if (ds18b20_get_temperature_raw(m_sensor, &temp) == ESP_OK) {
      return temp;
    }
    delay(50);
  }
  return DEVICE_DISCONNECTED_RAW;
}

/**
 * \brief Reads the temperature.
 *
 * If successful, constrains the temp to the range of the temperature type
 * and updates lastRequestTime. If unsuccessful, leaves lastRequestTime alone
 * and returns TEMP_SENSOR_DISCONNECTED.
 */
temperature OneWireTempSensor::readAndConstrainTemp() {
  const uint8_t attempts = 10;
  temperature temp = readTempWithRetries(attempts);

  if (temp == DEVICE_DISCONNECTED_RAW) {
    setConnected(false);
    return TEMP_SENSOR_DISCONNECTED;
  }

  const uint8_t shift = TEMP_FIXED_POINT_BITS - sensorPrecision;
  temp = constrainTemp(temp + m_calibration_offset + (C_OFFSET >> shift),
                       ((int)MIN_TEMP) >> shift,
                       ((int)MAX_TEMP) >> shift) << shift;
  return temp;
}
#endif // ESP8266