/**
 * @file axp192.cpp
 * @brief ESP-IDF compatible AXP192 power management driver implementation
 *
 * Uses ESP-IDF I2C master driver (driver_ng).
 */

#include "axp192.h"
#include <thorlog.h>
#include <cstring>

// AXP192 Register definitions
#define AXP192_REG_POWER_STATUS     0x00
#define AXP192_REG_MODE_CHGSTATUS   0x01
#define AXP192_REG_OTG_STATUS       0x02
#define AXP192_REG_IC_TYPE          0x03
#define AXP192_REG_EXTEN_DCDC2_CTL  0x10
#define AXP192_REG_DCDC1_VOLTAGE    0x26
#define AXP192_REG_DCDC2_VOLTAGE    0x27
#define AXP192_REG_DCDC3_VOLTAGE    0x23
#define AXP192_REG_LDO23_VOLTAGE    0x28
#define AXP192_REG_DCDC13_LDO23_CTL 0x12
#define AXP192_REG_GPIO0_CTL        0x90
#define AXP192_REG_GPIO0_VOLTAGE    0x91
#define AXP192_REG_GPIO1_CTL        0x92
#define AXP192_REG_GPIO2_CTL        0x93
#define AXP192_REG_GPIO34_CTL       0x95
#define AXP192_REG_BACKUP_CHG       0x35
#define AXP192_REG_VBUS_IPSOUT      0x30

// Timeout for I2C operations
#define I2C_TIMEOUT_MS  100

AXP192_Driver::AXP192_Driver(uint8_t addr)
    : m_addr(addr)
    , m_bus_handle(nullptr)
    , m_dev_handle(nullptr)
    , m_initialized(false)
{
}

esp_err_t AXP192_Driver::initI2C(int sda_pin, int scl_pin)
{
    // Configure the I2C master bus
    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = I2C_NUM_1;
    bus_config.sda_io_num = static_cast<gpio_num_t>(sda_pin);
    bus_config.scl_io_num = static_cast<gpio_num_t>(scl_pin);
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&bus_config, &m_bus_handle);
    if (err != ESP_OK) {
        Log.error("AXP192: Failed to create I2C master bus: %d" CR, err);
        return err;
    }

    // Add the AXP192 device to the bus
    i2c_device_config_t dev_config = {};
    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = m_addr;
    dev_config.scl_speed_hz = 400000;  // 400kHz

    err = i2c_master_bus_add_device(m_bus_handle, &dev_config, &m_dev_handle);
    if (err != ESP_OK) {
        Log.error("AXP192: Failed to add device to I2C bus: %d" CR, err);
        i2c_del_master_bus(m_bus_handle);
        m_bus_handle = nullptr;
        return err;
    }

    return ESP_OK;
}

void AXP192_Driver::deinitI2C()
{
    if (m_dev_handle != nullptr) {
        i2c_master_bus_rm_device(m_dev_handle);
        m_dev_handle = nullptr;
    }
    if (m_bus_handle != nullptr) {
        i2c_del_master_bus(m_bus_handle);
        m_bus_handle = nullptr;
    }
}

esp_err_t AXP192_Driver::writeRegister(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(m_dev_handle, data, 2, I2C_TIMEOUT_MS);
}

esp_err_t AXP192_Driver::readRegister(uint8_t reg, uint8_t* value)
{
    return i2c_master_transmit_receive(m_dev_handle, &reg, 1, value, 1, I2C_TIMEOUT_MS);
}

esp_err_t AXP192_Driver::setBits(uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current;
    esp_err_t err = readRegister(reg, &current);
    if (err != ESP_OK) {
        return err;
    }

    current = (current & ~mask) | (value & mask);
    return writeRegister(reg, current);
}

bool AXP192_Driver::detect(int sda_pin, int scl_pin)
{
    // Initialize I2C
    esp_err_t err = initI2C(sda_pin, scl_pin);
    if (err != ESP_OK) {
        return false;
    }

    // Try to read the IC type register
    uint8_t ic_type = 0;
    err = readRegister(AXP192_REG_IC_TYPE, &ic_type);

    // Clean up I2C
    deinitI2C();

    if (err == ESP_OK) {
        Log.notice("AXP192: Detected IC type 0x%02X" CR, ic_type);
        return true;
    }

    return false;
}

bool AXP192_Driver::begin(int sda_pin, int scl_pin, const AXP192_InitDef& init)
{
    // Initialize I2C
    esp_err_t err = initI2C(sda_pin, scl_pin);
    if (err != ESP_OK) {
        return false;
    }

    // Verify device is present
    uint8_t ic_type = 0;
    err = readRegister(AXP192_REG_IC_TYPE, &ic_type);
    if (err != ESP_OK) {
        Log.error("AXP192: Device not responding" CR);
        deinitI2C();
        return false;
    }

    Log.notice("AXP192: Initializing (IC type 0x%02X)" CR, ic_type);

    m_initialized = true;

    // Configure EXTEN (external power output)
    setEXTEN(init.EXTEN);

    // Configure backup battery charging
    setBACKUP(init.BACKUP);

    // Configure DC-DC converters
    setDCDC1(init.DCDC1);
    setDCDC2(init.DCDC2);
    setDCDC3(init.DCDC3);

    // Configure LDO outputs
    setLDO2(init.LDO2);
    setLDO3(init.LDO3);

    // Configure GPIO0 (LDO-like voltage output)
    setGPIO0(init.GPIO0);

    // GPIO1-4 are typically used as digital outputs in M5StickC
    // Skip if set to -1

    Log.notice("AXP192: Initialization complete" CR);
    return true;
}

void AXP192_Driver::end()
{
    if (m_initialized) {
        deinitI2C();
        m_initialized = false;
    }
}

void AXP192_Driver::setDCDC(uint8_t channel, int16_t voltage_mv)
{
    if (!m_initialized) return;

    uint8_t enable_bit = 0;
    uint8_t voltage_reg = 0;
    int16_t min_mv = 700;
    int16_t max_mv = 3500;
    int16_t step_mv = 25;

    switch (channel) {
        case 1:
            enable_bit = 0x01;  // Bit 0 of reg 0x12
            voltage_reg = AXP192_REG_DCDC1_VOLTAGE;
            break;
        case 2:
            enable_bit = 0x10;  // Bit 4 of reg 0x12
            voltage_reg = AXP192_REG_DCDC2_VOLTAGE;
            max_mv = 2275;
            break;
        case 3:
            enable_bit = 0x02;  // Bit 1 of reg 0x12
            voltage_reg = AXP192_REG_DCDC3_VOLTAGE;
            break;
        default:
            return;
    }

    if (voltage_mv <= 0) {
        // Disable this DCDC
        setBits(AXP192_REG_DCDC13_LDO23_CTL, enable_bit, 0);
        return;
    }

    // Clamp voltage
    if (voltage_mv < min_mv) voltage_mv = min_mv;
    if (voltage_mv > max_mv) voltage_mv = max_mv;

    // Calculate register value
    uint8_t reg_val = static_cast<uint8_t>((voltage_mv - min_mv) / step_mv);

    // Set voltage
    writeRegister(voltage_reg, reg_val);

    // Enable output
    setBits(AXP192_REG_DCDC13_LDO23_CTL, enable_bit, enable_bit);
}

void AXP192_Driver::setDCDC1(int16_t voltage_mv)
{
    setDCDC(1, voltage_mv);
}

void AXP192_Driver::setDCDC2(int16_t voltage_mv)
{
    setDCDC(2, voltage_mv);
}

void AXP192_Driver::setDCDC3(int16_t voltage_mv)
{
    setDCDC(3, voltage_mv);
}

void AXP192_Driver::setLDO(uint8_t channel, int16_t voltage_mv)
{
    if (!m_initialized) return;

    uint8_t enable_bit = 0;
    int16_t min_mv = 1800;
    int16_t max_mv = 3300;
    int16_t step_mv = 100;

    switch (channel) {
        case 2:
            enable_bit = 0x04;  // Bit 2 of reg 0x12
            break;
        case 3:
            enable_bit = 0x08;  // Bit 3 of reg 0x12
            break;
        default:
            return;
    }

    if (voltage_mv <= 0) {
        // Disable this LDO
        setBits(AXP192_REG_DCDC13_LDO23_CTL, enable_bit, 0);
        return;
    }

    // Clamp voltage
    if (voltage_mv < min_mv) voltage_mv = min_mv;
    if (voltage_mv > max_mv) voltage_mv = max_mv;

    // Calculate register value (0-15)
    uint8_t reg_val = static_cast<uint8_t>((voltage_mv - min_mv) / step_mv);

    // LDO2 and LDO3 share register 0x28
    // LDO2 is bits 7:4, LDO3 is bits 3:0
    uint8_t current;
    readRegister(AXP192_REG_LDO23_VOLTAGE, &current);

    if (channel == 2) {
        current = (current & 0x0F) | (reg_val << 4);
    } else {
        current = (current & 0xF0) | (reg_val & 0x0F);
    }

    writeRegister(AXP192_REG_LDO23_VOLTAGE, current);

    // Enable output
    setBits(AXP192_REG_DCDC13_LDO23_CTL, enable_bit, enable_bit);
}

void AXP192_Driver::setLDO2(int16_t voltage_mv)
{
    setLDO(2, voltage_mv);
}

void AXP192_Driver::setLDO3(int16_t voltage_mv)
{
    setLDO(3, voltage_mv);
}

void AXP192_Driver::setGPIO0(int16_t voltage_mv)
{
    if (!m_initialized) return;

    if (voltage_mv <= 0) {
        // Set GPIO0 to floating/disabled
        writeRegister(AXP192_REG_GPIO0_CTL, 0x07);  // NMOS open-drain
        return;
    }

    // GPIO0 can output LDO voltage (1800-3300mV in 100mV steps)
    int16_t min_mv = 1800;
    int16_t max_mv = 3300;
    int16_t step_mv = 100;

    if (voltage_mv < min_mv) voltage_mv = min_mv;
    if (voltage_mv > max_mv) voltage_mv = max_mv;

    uint8_t reg_val = static_cast<uint8_t>((voltage_mv - min_mv) / step_mv);

    // Set GPIO0 to LDO mode (0x02)
    writeRegister(AXP192_REG_GPIO0_CTL, 0x02);

    // Set voltage
    writeRegister(AXP192_REG_GPIO0_VOLTAGE, reg_val << 4);
}

void AXP192_Driver::setEXTEN(bool enable)
{
    if (!m_initialized) return;

    // EXTEN is bit 6 of register 0x12
    setBits(AXP192_REG_DCDC13_LDO23_CTL, 0x40, enable ? 0x40 : 0);
}

void AXP192_Driver::setBACKUP(bool enable)
{
    if (!m_initialized) return;

    // Backup battery charging control
    // Register 0x35: bit 7 = enable, bits 6:5 = voltage (3.1V default), bits 1:0 = current
    if (enable) {
        // Enable with 3.0V, 200uA
        writeRegister(AXP192_REG_BACKUP_CHG, 0xA2);
    } else {
        writeRegister(AXP192_REG_BACKUP_CHG, 0x22);
    }
}
