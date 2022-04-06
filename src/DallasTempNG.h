#ifndef BREWPI_ESP_DALLAS_TEMP_NG_H
#define BREWPI_ESP_DALLAS_TEMP_NG_H

#include <OneWire.h>
#include <DallasTemperature.h>

bool initConnection(DallasTemperature &sensor, const uint8_t* deviceAddress);
int16_t getTempRaw(DallasTemperature &sensor, const uint8_t* deviceAddress);

#endif  // BREWPI_ESP_DALLAS_TEMP_NG_H