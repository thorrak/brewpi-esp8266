#pragma once
#ifndef ESP8266

#include "onewire_bus.h"
#include "ds18b20.h"

#define DEVICE_DISCONNECTED_RAW -32768

esp_err_t ds18b20_get_temperature_raw(ds18b20_device_handle_t ds18b20, int16_t *ret_temperature);

#endif // ESP8266
