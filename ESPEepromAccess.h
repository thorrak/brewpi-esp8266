/*
* Copyright 2016 John Beeler
*
* This is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this file.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef ESP8266
// Generate an error if we have been incorrectly included in an Arduino build
#error Incorrect processor type!
#endif

#include <EEPROM.h>


//TODO - Clean this up
class ESPEepromAccess
{
public:
	static uint8_t readByte(eptr_t offset) {
		return EEPROM.read(offset);
//		return eeprom_read_byte((uint8_t*)offset);
	}
	static void writeByte(eptr_t offset, uint8_t value) {
		EEPROM.write(offset, value);
//		eeprom_update_byte((uint8_t*)offset, value);
	}

/*	static void readBlock(uint8_t* target, eptr_t offset, uint16_t size) {
//		eeprom_read_block(target, (uint8_t*)offset, size);
		int i;
		for (i = 0; i<size; i++) {
			target[i] = EEPROM.read(offset + i);
		}
	}
	static void writeBlock(eptr_t target, const uint8_t* source, uint16_t size) {
//		eeprom_update_block(source, (void*)target, size);
		int i;
		for (i = 0; i<size; i++) {
			EEPROM.write(target + i, source[i]);
		}
		EEPROM.commit();

	}*/

	static void readBlock(void* target, eptr_t offset, uint16_t size) {
		//		eeprom_read_block(target, (uint8_t*)offset, size);
		EEPROM.get(offset, target);
	}

	static void writeBlock(eptr_t target, const void* source, uint16_t size) {
		//		eeprom_update_block(source, (void*)target, size);
		EEPROM.put(target, source);
		EEPROM.commit();

	}

	static void commit(void) {
		EEPROM.commit();
	}

};
