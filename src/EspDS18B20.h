#pragma once

#include "onewire_bus.h"
#include "ds18b20.h"

#define DEVICE_DISCONNECTED_RAW -32768

/**
 * \brief Initialize connection and set up reset detection for a DS18B20 sensor
 *
 * This sets up a mechanism to detect if the sensor has been reset (power cycled).
 * It writes 0 to the HIGH_ALARM register in EEPROM, then writes 1 to RAM only.
 * On power cycle, the RAM value reverts to the EEPROM value (0), which can be
 * detected by ds18b20_get_temperature_raw() to indicate the sensor needs re-init.
 *
 * This should be called during sensor initialization.
 *
 * @param ds18b20 The DS18B20 device handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ds18b20_init_connection(ds18b20_device_handle_t ds18b20);

/**
 * \brief Read raw temperature from a DS18B20 sensor with reset detection
 *
 * Reads the temperature from the sensor's scratchpad. Also checks if the sensor
 * has been reset since the last init_connection() call. If reset is detected,
 * returns DEVICE_DISCONNECTED_RAW to indicate the sensor needs re-initialization.
 *
 * @param ds18b20 The DS18B20 device handle
 * @param ret_temperature Pointer to store the raw temperature value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ds18b20_get_temperature_raw(ds18b20_device_handle_t ds18b20, int16_t *ret_temperature);

/**
 * \brief Trigger temperature conversion on all DS18B20 sensors on the bus
 *
 * This is a non-blocking call that sends the convert command to all sensors
 * using Skip ROM addressing. Sensors will begin conversion immediately.
 * The caller should wait at least 750ms before reading temperatures.
 *
 * @param bus The OneWire bus handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ds18b20_trigger_all_conversions_no_wait(onewire_bus_handle_t bus);

