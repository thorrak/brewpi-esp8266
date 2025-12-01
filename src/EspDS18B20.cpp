#ifndef ESP8266
#include "EspDS18B20.h"
#include "onewire_crc.h"
#include "onewire_cmd.h"
#include <string.h>

#define DS18B20_CMD_READ_SCRATCHPAD   0xBE

static const char *TAG = "esp_ds18b20";

typedef struct  {
  uint8_t temp_lsb;
  uint8_t temp_msb;
  uint8_t th_user1;
  uint8_t tl_user2;
  uint8_t configuration;
  uint8_t _reserved1;
  uint8_t _reserved2;
  uint8_t _reserved3;
  uint8_t crc_value;
} __attribute__((packed)) ds18b20_scratchpad_t;

typedef struct ds18b20_device_t {
  onewire_bus_handle_t bus;
  onewire_device_address_t addr;
  uint8_t th_user1;
  uint8_t tl_user2;
  ds18b20_resolution_t resolution;
} ds18b20_device_t;

static esp_err_t ds18b20_send_command(ds18b20_device_handle_t ds18b20, uint8_t cmd)
{
  if (ds18b20->addr == 0) {
    uint8_t tx_buffer[2] = {ONEWIRE_CMD_SKIP_ROM, cmd};
    return onewire_bus_write_bytes(ds18b20->bus, tx_buffer, sizeof(tx_buffer));
  }

  uint8_t tx_buffer[10] = {0};
  tx_buffer[0] = ONEWIRE_CMD_MATCH_ROM;
  memcpy(&tx_buffer[1], &ds18b20->addr, sizeof(ds18b20->addr));
  tx_buffer[sizeof(ds18b20->addr) + 1] = cmd;

  return onewire_bus_write_bytes(ds18b20->bus, tx_buffer, sizeof(tx_buffer));
}

esp_err_t ds18b20_get_temperature_raw(ds18b20_device_handle_t ds18b20, int16_t *ret_temperature)
{
  if (!ds18b20 || !ret_temperature) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = onewire_bus_reset(ds18b20->bus);
  if (ret != ESP_OK) {
    *ret_temperature = DEVICE_DISCONNECTED_RAW;
    return ret;
  }

  ret = ds18b20_send_command(ds18b20, DS18B20_CMD_READ_SCRATCHPAD);
  if (ret != ESP_OK) {
    *ret_temperature = DEVICE_DISCONNECTED_RAW;
    return ret;
  }

  ds18b20_scratchpad_t scratchpad;
  ret = onewire_bus_read_bytes(ds18b20->bus, (uint8_t *)&scratchpad, sizeof(scratchpad));
  if (ret != ESP_OK) {
    *ret_temperature = DEVICE_DISCONNECTED_RAW;
    return ret;
  }

  if (onewire_crc8(0, (uint8_t *)&scratchpad, 8) != scratchpad.crc_value) {
    *ret_temperature = DEVICE_DISCONNECTED_RAW;
    return ESP_ERR_INVALID_CRC;
  }

  const uint8_t lsb_mask[4] = {0x07, 0x03, 0x01, 0x00};
  uint8_t lsb_masked = scratchpad.temp_lsb & (~lsb_mask[scratchpad.configuration >> 5]);
  *ret_temperature = (((int16_t)scratchpad.temp_msb << 8) | lsb_masked);

  return ESP_OK;
}
#endif // ESP8266
