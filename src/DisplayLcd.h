/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
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
#include "DisplayBase.h"

// If BREWPI_TFT is set, we're going to use the DisplayTFT headers instead
#ifdef BREWPI_TFT
#include "DisplayTFT.h"
#else

/*
 * \addtogroup display
 * @{
 */

#if BREWPI_EMULATE || !BREWPI_LCD
#include "NullLcdDriver.h"
	typedef NullLcdDriver LcdDriver;
#elif defined(BREWPI_OLED)
#include "OLEDFourBit.h"
	typedef OLEDFourBit LcdDriver;
#elif defined(BREWPI_IIC)
#include "IicLcd.h"
	typedef IIClcd      LcdDriver;
#elif defined(BREWPI_SHIFT_LCD)
#include "SpiLcd.h"
	typedef SpiLcd      LcdDriver;
#else
#error "Wrong LCD type! Select one in Config.h."
#endif


/**
 * \brief LCD Display Content
 *
 * This class handles the content of the LCD.  It dispatches to the various
 * hardware driver implementations to actually display the content
 */
class LcdDisplay DISPLAY_SUPERCLASS
{
  public:
	// initializes the lcd display
	DISPLAY_METHOD void init();

  /**
   * Print all display content
   */
	DISPLAY_METHOD void printAll() {
		printStationaryText();
		printState();
		printAllTemperatures();
		printMode();
		updateBacklight();
	}

	DISPLAY_METHOD void printAllTemperatures();

	// print the stationary text on the lcd.
	DISPLAY_METHOD void printStationaryText();

	// print mode on the right location on the first line, after Mode:
	DISPLAY_METHOD void printMode();

	DISPLAY_METHOD void setDisplayFlags(uint8_t newFlags);
	DISPLAY_METHOD uint8_t getDisplayFlags() { return flags; };

	// print beer temperature at the right place on the display
	DISPLAY_METHOD void printBeerTemp();

	// print beer temperature setting at the right place on the display
	DISPLAY_METHOD void printBeerSet();

	// print fridge temperature at the right place on the display
	DISPLAY_METHOD void printFridgeTemp();

	// print fridge temperature setting at the right place on the display
	DISPLAY_METHOD void printFridgeSet();

	// print the current state on the last line of the LCD
	DISPLAY_METHOD void printState();

	DISPLAY_METHOD void getLine(uint8_t lineNumber, char *buffer) { lcd.getLine(lineNumber, buffer); }

	DISPLAY_METHOD void printAt_P(uint8_t x, uint8_t y, const char *text);

	DISPLAY_METHOD void setBufferOnly(bool bufferOnly)
	{
		lcd.setBufferOnly(bufferOnly);
	}

	DISPLAY_METHOD void resetBacklightTimer() { lcd.resetBacklightTimer(); }
	DISPLAY_METHOD void updateBacklight() { lcd.updateBacklight(); }

	// print a temperature
	DISPLAY_METHOD void printTemperature(temperature temp);
	DISPLAY_METHOD void printTemperatureAt(uint8_t x, uint8_t y, temperature temp);

	// print degree sign + C/F
	DISPLAY_METHOD void printDegreeUnit(uint8_t x, uint8_t y);

	DISPLAY_METHOD void printAt(uint8_t x, uint8_t y, char *text);

#ifdef ESP8266_WiFi
	DISPLAY_METHOD void printWiFiStartup();
	DISPLAY_METHOD void printWiFi();
	DISPLAY_METHOD void printWiFi_setup();
	DISPLAY_METHOD void printWiFiConnect();
#endif

#ifdef HAS_BLUETOOTH
	DISPLAY_METHOD void printBluetoothStartup();
#endif

	DISPLAY_METHOD void clear();


  private:
	DISPLAY_FIELD LcdDriver lcd;
	DISPLAY_FIELD uint8_t stateOnDisplay;
	DISPLAY_FIELD uint8_t flags;
};

#endif
/** @} */
