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

#pragma once

/**
 * \defgroup display LCD Display
 * \brief Support for different LCD display types
 *
 * \addtogroup display
 * @{
 *
 * \file Display.h
 * \brief To use the display, include this file.
 *
 * It takes care of setting DisplayType to the appropriate type of display
 * according to the compile-time config.
 */

#include "displays/DisplayBase.h"

#ifdef BREWPI_IIC
// DisplayLCD covers both I2C and SPI LCD20004, as well as OLED 4 bit
#include "displays/DisplayLcd.h"
#elif defined(BREWPI_TFT_ILI9341)
#include "displays/DisplayTFT_ILI.h"
#elif defined(BREWPI_TFT_ESPI)
#include "displays/DisplayTFT_eSPI.h"
#else
#error "Must select at least one valid display type!"
#endif

typedef LcdDisplay DisplayType;


extern DisplayType DISPLAY_REF display;

/** @} */
