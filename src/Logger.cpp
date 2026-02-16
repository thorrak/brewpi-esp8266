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
#include "BrewpiStrings.h"
#include "Logger.h"
#include "PiLink.h"
#include "TemperatureFormats.h"


/**
 * \brief Log a message
 *
 * \param type - Type of message
 * \param errorID
 */
void Logger::logMessageVaArg(const char type, const LOG_ID_TYPE errorID, const char * varTypes, ...){
  JsonDocument doc;

	va_list args;
  { char typeStr[2] = { type, '\0' }; doc["logType"] = typeStr; }
  doc["logID"] = errorID;
  JsonArray varArray = doc["V"].to<JsonArray>();

	va_start (args, varTypes);
	uint8_t index = 0;
	char buf[13];
	while(varTypes[index]){
		switch(varTypes[index]){
			case 'd': // integer, signed or unsigned
        varArray.add(va_arg(args, int));
				break;
			case 's': // string
			{
				char str_copy[64];
				strlcpy(str_copy, va_arg(args, char*), sizeof(str_copy));
				varArray.add(str_copy);
			}
				break;
			case 't': // temperature in fixed_7_9 format
			{
				char temp_str[13];
				strlcpy(temp_str, tempToString(buf, va_arg(args,int), 1, 12), sizeof(temp_str));
				varArray.add(temp_str);
			}
			break;
			case 'f': // fixed point value
			{
				char fp_str[13];
				strlcpy(fp_str, fixedPointToString(buf, (temperature) va_arg(args,int), 3, 12), sizeof(fp_str));
				varArray.add(fp_str);
			}
			break;
		}

    index++;
	}
	va_end (args);
  piLink.sendJsonMessage('D', doc);
}

Logger logger;
