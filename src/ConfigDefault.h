/*
 * Copyright 2013 BrewPi/Elco Jacobs.
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

/**
 * \file ConfigDefault.h
 * \brief Compile time configuration flags
 */

/**
 * Do not change this file directly - rather edit Config.h
 */

#ifndef BREWPI_STATIC_CONFIG
#define BREWPI_STATIC_CONFIG BREWPI_SHIELD_REV_C
#endif

/*
 * LCD Display using a shift register.
 * For diy-shields prior to the revA shield, this should be set to 0.
 */
#ifndef BREWPI_SHIFT_LCD
#if BREWPI_STATIC_CONFIG != BREWPI_SHIELD_DIY
	#define BREWPI_SHIFT_LCD 1
#else
	#define BREWPI_SHIFT_LCD 0
#endif
#endif

// Enable printing debug only log messages and debug only wrapped statements
#ifndef BREWPI_DEBUG
#define BREWPI_DEBUG 0
#endif

#if BREWPI_DEBUG>0
	#define DEBUG_ONLY(x) x
#else
	#define DEBUG_ONLY(x)
#endif


// Set which debug messages are printed
#ifndef BREWPI_LOG_ERRORS
#define BREWPI_LOG_ERRORS 1
#endif

#ifndef BREWPI_LOG_WARNINGS
#define BREWPI_LOG_WARNINGS 1
#endif

#ifndef BREWPI_LOG_INFO
#define BREWPI_LOG_INFO 1
#endif

#ifndef BREWPI_LOG_DEBUG
#define BREWPI_LOG_DEBUG 0
#endif

/**
 * \def BREWPI_EMULATE
 * \brief Enable virtual hardware
 * This flag virtualizes as much of the hardware as possible, so the code can be run in the AvrStudio simulator, which
 * only emulates the microcontroller, not any attached peripherals.
 */
#ifndef BREWPI_EMULATE
#define BREWPI_EMULATE 0
#endif

#ifndef TEMP_SENSOR_CASCADED_FILTER
#define TEMP_SENSOR_CASCADED_FILTER 1
#endif

#ifndef TEMP_CONTROL_STATIC
#define TEMP_CONTROL_STATIC 1
#endif

#ifndef FAST_DIGITAL_PIN 
#define FAST_DIGITAL_PIN 0
#endif

/**
 * Enable the simulator. Real sensors/actuators are replaced with simulated versions. In particular, the values reported by
 * temp sensors are based on a model of the fridge/beer.
 */
#ifndef BREWPI_SIMULATE
#define BREWPI_SIMULATE 0
#endif

/**
 * \def BREWPI_DS2413
 * \brief Enable DS2413 Actuators.
 * \ingroup hardware
 */
#ifndef BREWPI_DS2413
#define BREWPI_DS2413 0
#endif

/**
 * Enable the LCD menu.
 */
#ifndef BREWPI_MENU
#define BREWPI_MENU 1
#endif

/**
 * \def BREWPI_LCD
 * \brief Enable the LCD display and select its type.
 *
 * Without this, a NullDisplay is used.
 * \ingroup display
 */
#ifndef BREWPI_LCD
#define BREWPI_LCD 1

 // #define BREWPI_IIC       1      // LCD display connected to I2C.
 // #define BREWPI_OLED      1

/**
 * \def BREWPI_SHIFT_LCD
 * \brief LCD is attached to SPI bus (using level shifters)
 *
 * \ingroup display
 */
#define BREWPI_SHIFT_LCD 1

#endif // endif BREWPI_LCD


/**
 * \def BACKLIGHT_AUTO_OFF_PERIOD
 * \brief Turn off LCD backlight after this time. Seconds.
 *
 * Set to 0 to disable and leave backlight on.
 * \ingroup display
 */
#ifndef BACKLIGHT_AUTO_OFF_PERIOD
#define BACKLIGHT_AUTO_OFF_PERIOD 60
#endif

#ifndef BREWPI_BUZZER
	#if BREWPI_STATIC_CONFIG==BREWPI_SHIELD_DIY
		#define BREWPI_BUZZER 0
	#else
		#define BREWPI_BUZZER 1
	#endif
#endif

#ifndef BREWPI_ROTARY_ENCODER
#if BREWPI_STATIC_CONFIG==BREWPI_SHIELD_DIY
	#define BREWPI_ROTARY_ENCODER 0
#else
	#define BREWPI_ROTARY_ENCODER 1
#endif	
#endif

#ifndef BREWPI_EEPROM_HELPER_COMMANDS
#define BREWPI_EEPROM_HELPER_COMMANDS BREWPI_DEBUG || BREWPI_SIMULATE
#endif

/**
 * \def BREWPI_SENSOR_PINS
 * \brief Enable sensor pins.
 * This can be disable only if all sensors are OneWire devices.
 * \ingroup hardware
 */
#ifndef BREWPI_SENSOR_PINS
#define BREWPI_SENSOR_PINS 1
#endif

/**
 * \def BREWPI_ACTUATOR_PINS
 * \brief Enable actuator pins.
 * This can be disable only if all actuators are OneWire devices.
 * \ingroup hardware
 */
#ifndef BREWPI_ACTUATOR_PINS
#define BREWPI_ACTUATOR_PINS 1
#endif


#ifndef BREWPI_BOARD

#if !ARDUINO
        #define BREWPI_BOARD BREWPI_BOARD_UNKNOWN
#elif defined(__AVR_ATmega32U4__)
        #define BREWPI_BOARD BREWPI_BOARD_LEONARDO
#elif defined(__AVR_ATmega328P__)
        #define BREWPI_BOARD BREWPI_BOARD_STANDARD
#elif defined(__AVR_ATmega2560__)
        #define BREWPI_BOARD BREWPI_BOARD_MEGA
#elif defined(ESP8266)
		#define BREWPI_BOARD BREWPI_BOARD_ESP8266
#elif defined(ESP32_STOCK)
		#define BREWPI_BOARD BREWPI_BOARD_ESP32
#elif defined(ESP32C3)
		#define BREWPI_BOARD BREWPI_BOARD_ESP32C3
#elif defined(ESP32S2)
		#define BREWPI_BOARD BREWPI_BOARD_ESP32S2
#else
        #error Unknown processor type!
        #define BREWPI_BOARD BREWPI_BOARD_UNKNOWN
#endif

#endif // ifndef BREWPI_BOARD

#ifndef OPTIMIZE_GLOBAL
#define OPTIMIZE_GLOBAL 1
#endif

/**
 * \def ONEWIRE_CRC8_TABLE
 * \brief Disable OneWire CRC table
 * The table takes up 256 bytes of progmem.
 * \ingroup hardware
 */
#ifndef ONEWIRE_CRC8_TABLE
#define ONEWIRE_CRC8_TABLE 0
#endif

#ifndef DISPLAY_TIME_HMS
#define DISPLAY_TIME_HMS 1
#endif

/**
 * \def ONEWIRE_PARASITE_SUPPORT
 * \brief Enable OneWire parasite power
 * If the OneWire bus is using parasite power, the internal pull up resistors
 * need to be enabled.  This flag controls that.
 * \ingroup hardware
 */
#ifndef ONEWIRE_PARASITE_SUPPORT
#define ONEWIRE_PARASITE_SUPPORT 0
#endif

#ifndef DS2413_SUPPORT_SENSE
#define DS2413_SUPPORT_SENSE 0
#endif


