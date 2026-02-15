#pragma once

#include <LovyanGFX.hpp>

#if defined(BREWPI_TFT_ILI9341)

// ILI9341 320x240 TFT on LoLin D32 Pro
// Matches LovyanGFX autodetect config for board_LoLinD32 (ILI9341)
// Pins: CS=14, DC=27, RST=33, Backlight=32, Touch CS=12
// SPI: SCLK=18, MOSI=23, MISO=19 (VSPI_HOST)
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;
    lgfx::Touch_XPT2046 _touch_instance;

public:
    LGFX(void)
    {
        // SPI bus configuration
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI3_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = true;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 18;
            cfg.pin_mosi = 23;
            cfg.pin_miso = 19;
            cfg.pin_dc = TFT_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel configuration
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = TFT_CS;
            cfg.pin_rst = TFT_RST;
            cfg.pin_busy = -1;
            cfg.memory_width = 240;
            cfg.memory_height = 320;
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
        }

        // Backlight (PWM on GPIO 32)
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = TFT_BACKLIGHT;
            cfg.pwm_channel = 7;
            cfg.freq = 44100;
            cfg.invert = false;
            _light_instance.config(cfg);
            _panel_instance.light(&_light_instance);
        }

        // Touch controller (XPT2046 sharing SPI bus, CS on GPIO 12)
        {
            auto cfg = _touch_instance.config();
            cfg.bus_shared = true;
            cfg.spi_host = SPI3_HOST;
            cfg.pin_cs = TS_CS;
            cfg.pin_mosi = 23;
            cfg.pin_miso = 19;
            cfg.pin_sclk = 18;
            cfg.offset_rotation = 2;
            _touch_instance.config(cfg);
            _panel_instance.touch(&_touch_instance);
        }

        setPanel(&_panel_instance);
    }
};

#elif defined(BREWPI_TFT_ESPI)

// ST7789 135x240 TFT on M5StickC Plus (esp32dev)
// Pins: CS=5, DC=23, RST=18, MOSI=15, SCLK=13 (SPI2_HOST)
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX(void)
    {
        // SPI bus configuration
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 27000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = true;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 13;
            cfg.pin_mosi = 15;
            cfg.pin_miso = -1;
            cfg.pin_dc = 23;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel configuration
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 5;
            cfg.pin_rst = 18;
            cfg.pin_busy = -1;
            cfg.memory_width = 240;
            cfg.memory_height = 320;
            cfg.panel_width = 135;
            cfg.panel_height = 240;
            cfg.offset_x = 52;
            cfg.offset_y = 40;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = true;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};

#endif
