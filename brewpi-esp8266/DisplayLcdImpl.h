/*
* DisplayLcdImpl.h
*
* Created: 07/01/2015 06:01:02
*  Author: mat
*/

#pragma once


#if BREWPI_EMULATE || !BREWPI_LCD || !ARDUINO
#include "NullLcdDriver.h"
typedef NullLcdDriver LcdDriver;
#elif defined(BREWPI_OLED)
#include "OLEDFourBit.h"
typedef OLEDFourBit LcdDriver;
#elif defined(BREWPI_IIC)  // Merging DisplayLcd code into DisplayLcdImpl.h -- Thorrak
#include "IicLcd.h"
typedef IIClcd      LcdDriver;
#elif defined(BREWPI_SHIFT_LCD)
#include "SpiLcd.h"
typedef SpiLcd      LcdDriver;
#else
#error "Wrong LCD type! Select one in Config.h."
#endif

