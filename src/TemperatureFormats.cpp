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

#include "Brewpi.h"
#include "TemperatureFormats.h"
#include <string.h>
#include <limits.h>
#include "TempControl.h"

/**
 * Provide missing strchrnul on ESP8266
 *
 * @see https://linux.die.net/man/3/strchrnul
 */
char *
strchrnul(const char *s, int c_in)
{
	char c = c_in;
	while (*s && (*s != c))
		s++;

	return (char *)s;
}


// See header file for details about the temp format used.

/**
 * \brief Format temperature value as C-str
 *
 * Result can have maximum length of : sign + 3 digits integer part + point + 3
 * digits fraction part + '\0' = 9 bytes;
 * Only 1, 2 or 3 decimal places allowed.
 *
 * \param s - Buffer string to write result to
 * \param rawValue - temperature value
 * \param numDecimals - Precision to use when converting
 * \param maxLength - Size of buffer string
 * \returns pointer to the buffer string
 */
char * tempToString(char * s, long_temperature rawValue, uint8_t numDecimals, uint8_t maxLength){
	if(rawValue == INVALID_TEMP){
		strcpy_P(s, PSTR("null"));
		return s;
	}
	rawValue = convertFromInternalTemp(rawValue);
	return fixedPointToString(s, rawValue, numDecimals, maxLength);
}


/**
 * \brief Format temperature diff value as C-str
 *
 * \param s - Buffer string to write result to
 * \param rawValue - temperature value
 * \param numDecimals - Precision to use when converting
 * \param maxLength - Size of buffer string
 * \returns pointer to the buffer string
 */
char * tempDiffToString(char * s, long_temperature rawValue, uint8_t numDecimals, uint8_t maxLength){
	rawValue = convertFromInternalTempDiff(rawValue);
	return fixedPointToString(s, rawValue, numDecimals, maxLength);
}


/**
 * \brief Format fixed point value as C-str
 *
 * \param s - Buffer string to write result to
 * \param rawValue - fixed point value
 * \param numDecimals - Precision to use when converting
 * \param maxLength - Size of buffer string
 * \returns pointer to the buffer string
 */
char * fixedPointToString(char * s, temperature rawValue, uint8_t numDecimals, uint8_t maxLength){
	return fixedPointToString(s, long_temperature(rawValue), numDecimals, maxLength);
}

// this gets rid of snprintf_P
void mysnprintf_P(char* buf, int len, const char* fmt, ...)
{
	va_list args;
	va_start (args, fmt );
	vsnprintf_P(buf, len, fmt, args);
	va_end (args);
}


/**
 * \brief Format fixed point value as C-str
 *
 * \param s - Buffer string to write result to
 * \param rawValue - fixed point value
 * \param numDecimals - Precision to use when converting
 * \param maxLength - Size of buffer string
 * \returns pointer to the buffer string
 */
char * fixedPointToString(char * s, long_temperature rawValue, uint8_t numDecimals, uint8_t maxLength){
	s[0] = ' ';
	bool negative = false;
	if(rawValue < 0l){
		s[0] = '-';
		rawValue = -rawValue;
		negative = true;
	}
	
	int intPart = longTempDiffToInt(rawValue); // rawValue is supposed to be without internal offset
	uint16_t fracPart;
	const char* fmt;
	uint16_t scale;
	switch (numDecimals)
	{
		case 1:
			fmt = PSTR("%d.%01d");
			scale = 10;
			break;
		case 2:
			fmt = PSTR("%d.%02d");
			scale = 100;
			break;
		default:
			fmt = PSTR("%d.%03d");
			scale = 1000;
	}
	fracPart = ((rawValue & TEMP_FIXED_POINT_MASK) * scale + TEMP_FIXED_POINT_SCALE/2) >> TEMP_FIXED_POINT_BITS; // add 256 for rounding
	if(fracPart >= scale){
		intPart++;
		fracPart = 0;
	}
	if(negative)
		mysnprintf_P(&s[1], maxLength-1, fmt,  intPart, fracPart);
	else
		mysnprintf_P(&s[0], maxLength, fmt,  intPart, fracPart);
	return s;
}


/**
 * \brief Convert a c-str into a temperature value
 *
 * \param numberString - String to convert (Either with units or in tempControl.cc.tempFormat units)
 * \returns converted temperature
 */
temperature stringToTemp(const char * numberString){
    char tempBuffer[20];
    int bufferIdx = 0;
    // char tempFormat = '\0';
	char tempFormat = tempControl.cc.tempFormat;

    for (int i = 0; numberString[i] != '\0' && i < 19; i++) {
        if (numberString[i] == ' ') {
            tempFormat = numberString[i+1];  // Capture the unit after the space.
            break;
        } else {
            tempBuffer[i] = numberString[i];
            tempBuffer[i+1] = '\0';
        }
    }

    long_temperature rawTemp = stringToFixedPoint(tempBuffer);
    
    // if (tempFormat != '\0') {
        rawTemp = convertToInternalTemp(rawTemp, tempFormat);
    // } else {
        // rawTemp = convertToInternalTemp(rawTemp);
    // }

    return constrainTemp16(rawTemp);
}



/**
 * \brief Convert a c-str into a temperature diff
 *
 * \param numberString - String to convert
 * \returns converted temperature diff
 */
temperature stringToTempDiff(const char * numberString){
	long_temperature rawTempDiff = stringToFixedPoint(numberString);
	rawTempDiff = convertToInternalTempDiff(rawTempDiff);
	return constrainTemp16(rawTempDiff);
}


/**
 * \brief Convert a c-str into a fixed point
 *
 * \param numberString - String to convert
 * \returns converted fixed point temperature
 */
long_temperature stringToFixedPoint(const char * numberString){
	// receive new temperature as null terminated string: "19.20"
	long_temperature intPart = 0;
	long_temperature fracPart = 0;
	
	char * fractPtr = 0; //pointer to the point in the string
	bool negative = 0;
	if(numberString[0] == '-'){
		numberString++;
		negative = true; // by processing the sign here, we don't have to include strtol
	}
	
	// find the point in the string to split in the integer part and the fraction part
	fractPtr = strchrnul(numberString, '.'); // returns pointer to the point.
	
	intPart = atol(numberString);
	if(fractPtr != 0){
		// decimal point was found
		fractPtr++; // add 1 to pointer to skip point
		int8_t numDecimals = (int8_t) strlen(fractPtr);
		fracPart = atol(fractPtr);
		fracPart = fracPart << TEMP_FIXED_POINT_BITS; // bits for fraction part
		while(numDecimals > 0){
			fracPart = (fracPart + 5) / 10; // divide by 10 rounded
			numDecimals--;
		}
	}
	long_temperature absVal = (intPart << TEMP_FIXED_POINT_BITS) + fracPart;
	return negative ? -absVal:absVal;
}

/**
 * Receives the tempFormat temp format in fixed point and converts it to the internal format
 * It scales the value for Fahrenheit and adds the offset needed for absolute
 * temperatures. For temperature differences, use no offset.
 *
 * @param rawTemp - Temperature to convert
 * @param addOffset - Flag to control if temp offsets are added.  Should be
 * `true` when working with absolute temps, `false` when working with
 * differences.
 * @param tempFormat - The format of the temperature to convert
 */
long_temperature convertToInternalTempImpl(long_temperature rawTemp, bool addOffset, char tempFormat){
	if(tempFormat == 'F'){ // value received is in F, convert to C
		rawTemp = (rawTemp) * 5 / 9;
		if(addOffset){
			rawTemp += F_OFFSET;
		}
	}
	else{
		if(addOffset){
			rawTemp += C_OFFSET;
		}
	}
	return rawTemp;
}

/**
 * Receives the external temp format in fixed point and converts it to the internal format
 * It scales the value for Fahrenheit and adds the offset needed for absolute
 * temperatures. For temperature differences, use no offset.
 *
 * @param rawTemp - Temperature to convert
 * @param addOffset - Flag to control if temp offsets are added.  Should be
 * `true` when working with absolute temps, `false` when working with
 * differences.
 */
long_temperature convertToInternalTempImpl(long_temperature rawTemp, bool addOffset){
	return convertToInternalTempImpl(rawTemp, addOffset, tempControl.cc.tempFormat);
}

// convertAndConstrain adds an offset, then scales with *9/5 for Fahrenheit. Use it without the offset argument for temperature differences
long_temperature convertFromInternalTempImpl(long_temperature rawTemp, bool addOffset){
	if(tempControl.cc.tempFormat == 'F'){ // value received is in F, convert to C
		if(addOffset){
			rawTemp -= F_OFFSET;
		}
		rawTemp = rawTemp * 9 / 5;
	}
	else{
		if(addOffset){
			rawTemp -= C_OFFSET;
		}
	}
	return rawTemp;
}

/**
 * \brief Convert a fixed point temperature to decimal value rounded to nearest tenth
 *
 * @param temp - Temperature to convert
 */
int fixedToTenths(long_temperature temp){
	temp = convertFromInternalTemp(temp);
	return (int) ((10 * temp + intToTempDiff(5)/10) / intToTempDiff(1)); // return rounded result in tenth of degrees
}


/**
 * \brief Convert a decimal value temperature to the internal representation
 *
 * \param temp - Temperature to convert
 */
temperature tenthsToFixed(int temp){
	long_temperature fixedPointTemp = convertToInternalTemp(((long_temperature) temp * intToTempDiff(1) + 5) / 10);
	return constrainTemp16(fixedPointTemp);
}

/**
 * \brief Constrain a temp within bounds
 *
 * @param valLong - Temperature to constrain
 * @param lower - Lower bound
 * @param upper - Upper bound
 * @return The value of `valLong` if it is within the bounds.  `lower` if `valLong` is below, `upper` if `valLong` is above.
 */
temperature constrainTemp(long_temperature valLong, temperature lower, temperature upper){
	temperature val = constrainTemp16(valLong);

	if(val < lower){
		return lower;
	}

	if(val > upper){
		return upper;
	}
	return temperature(valLong);
}


/**
 * \brief Constrain a temp within bounds of MIN_TEMP and MAX_TEMP
 *
 * @param val - Temperature to constrain
 * @return The value of `val` if it is within the bounds.  `MIN_TEMP` if `val` is below, `MAX_TEMP` if `val` is above.
 * @see MIN_TEMP
 * @see MAX_TEMP
 */
temperature constrainTemp16(long_temperature val)
{
	if(val<MIN_TEMP){
		return MIN_TEMP;
	}
	if(val>MAX_TEMP){
		return MAX_TEMP;
	}
	return val;
}

temperature multiplyFactorTemperatureLong(temperature factor, long_temperature b)
{
	return constrainTemp16(((long_temperature) factor * (b-C_OFFSET))>>TEMP_FIXED_POINT_BITS);
}

temperature multiplyFactorTemperatureDiffLong(temperature factor, long_temperature b)
{
	return constrainTemp16(((long_temperature) factor * b)>>TEMP_FIXED_POINT_BITS);
}


temperature multiplyFactorTemperature(temperature factor, temperature b)
{
	return constrainTemp16(((long_temperature) factor * ((long_temperature) b - C_OFFSET))>>TEMP_FIXED_POINT_BITS);
}

temperature multiplyFactorTemperatureDiff(temperature factor, temperature b)
{
	return constrainTemp16(((long_temperature) factor * (long_temperature) b )>>TEMP_FIXED_POINT_BITS);
}


/**
 * \brief Convert a temperature to a double.
 * Used for JSON conversion.
 *
 * \param rawTemp - Temperature value to convert
 * \param numDecimals - Number of decimal places to include
 * \todo Do a direct conversion instead of going to string
 */
double tempToDouble(long_temperature rawTemp, uint8_t numDecimals) {
  char tempString[Config::TempFormat::bufferLen];
  String temp(tempToString(tempString, rawTemp, numDecimals, Config::TempFormat::bufferLen));
  temp.trim();
  return temp.toDouble();
}

/**
 * \brief Convert a temperature difference to a double.
 * Used for JSON conversion.
 *
 * \param rawTempDiff - Temperature difference value to convert
 * \param numDecimals - Number of decimal places to include
 * \todo Do a direct conversion instead of going to string
 */
double tempDiffToDouble(long_temperature rawTempDiff, uint8_t numDecimals) {
    char tempString[Config::TempFormat::bufferLen];
    String temp(tempDiffToString(tempString, rawTempDiff, numDecimals, Config::TempFormat::bufferLen));
    temp.trim();
    return temp.toDouble();
}

/**
 * \brief Convert a temperature to a double.
 * Used for JSON conversion.
 *
 * \param rawFixedPoint - Fixed point value to convert
 * \param numDecimals - Number of decimal places to include
 * \todo Do a direct conversion instead of going to string
 */
double fixedPointToDouble(long_temperature rawFixedPoint, uint8_t numDecimals) {
    char tempString[Config::TempFormat::bufferLen];
    String temp(fixedPointToString(tempString, rawFixedPoint, numDecimals, Config::TempFormat::bufferLen));
    temp.trim();
    return temp.toDouble();
}
