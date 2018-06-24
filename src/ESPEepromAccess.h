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

#include <FS.h>
#include "EepromStructs.h"
#include "EepromFormat.h"



#define SPIFFS_controlConstants_fname "/controlConstants"
#define SPIFFS_controlSettings_fname "/controlSettings"
#define SPIFFS_device_fname_prepend "/dev"

#define MAX_SPIFFS_DEVICES EepromFormat::MAX_DEVICES

//TODO - Clean this up
class ESPEepromAccess
{
private:
	template <class T> static bool writeBlockToFile(String target_name, T& data) {
		File out_file = SPIFFS.open(target_name, "w");
		if (out_file) {
			out_file.write((const uint8_t*)&data, sizeof(data));
			out_file.close();
			return true;
		} else {
			// TODO - log this
			return false;
		}
	}

	template <class T> static bool readBlockFromFile(String target_name, T& data) {
		// This creates some issues, as there is an assumption in most places readBlockFromFile is used that it will
		// always overwrite &data. Need to go back & re-read where it gets used to make sure we are still OK
		if(!doesFileExist(target_name))
			return false;

		File in_file = SPIFFS.open(target_name, "r");
		if (in_file) {
			uint8_t holding[sizeof(data)];
			in_file.read(holding, sizeof(data));
			memcpy(&data, holding, sizeof(data));
			in_file.close();
			return true;
		}
        return false;
	}

    static bool doesFileExist(String target_name) {
		return SPIFFS.exists(target_name);
    }


public:
	// Since we're basically switching to using SPIFFS for everything, I don't want these to compile
/*	static uint8_t readByte(eptr_t offset) {
		return EEPROM.read(offset);
	}
	static void writeByte(eptr_t offset, uint8_t value) {
		EEPROM.write(offset, value);
	}*/

    // TODO - Move this
    static void clear(uint8_t* p, uint8_t size) {
        while (size-->0) *p++ = 0;
    }

	static void readControlSettings(ControlSettings& target, eptr_t offset, uint16_t size) {
		/* Unlike readDeviceDefinition, controlSettings & controlConstants are both properly initialized elsewhere.
		 * Regardless, due to the tweak we made to readBlockFromFile, I'm going to explicitly add a call here to clear
		 * the memory just in case. */

		if(!readBlockFromFile(SPIFFS_controlSettings_fname, target))
			clear((uint8_t*)&target, sizeof(target));  // This mimics the behavior where previously the EEPROM would have been 0ed out.
	}

	static void readControlConstants(ControlConstants& target, eptr_t offset, uint16_t size) {
        if(!readBlockFromFile(SPIFFS_controlConstants_fname, target))
            clear((uint8_t*)&target, sizeof(target));
	}


	static void readDeviceDefinition(DeviceConfig& target, int8_t deviceID, uint16_t size) {
        char buf[20];
        sprintf(buf, "%s%d", SPIFFS_device_fname_prepend, deviceID);
        if(!readBlockFromFile(buf, target)) // deviceID was previously an offset in memory - now it's a sequential #
			clear((uint8_t*)&target, sizeof(target));  // This mimics the behavior where previously the EEPROM would have been 0ed out.
	}

	static void writeControlSettings(eptr_t target, ControlSettings& source, uint16_t size) {
		writeBlockToFile(SPIFFS_controlSettings_fname, source);
	}

	static void writeControlConstants(eptr_t target, ControlConstants& source, uint16_t size) {
        writeBlockToFile(SPIFFS_controlConstants_fname, source);
	}

	static void writeDeviceDefinition(int8_t deviceID, const DeviceConfig& source, uint16_t size) {
        char buf[20];
        sprintf(buf, "%s%d", SPIFFS_device_fname_prepend, deviceID);
        writeBlockToFile(buf, source);  // deviceID was previously an offset in memory - now it's a sequential #
//		logWarningIntString(0, sizeof(source), "writeDeviceDefinition called");
	}

    static bool hasSettings() {
        return doesFileExist(SPIFFS_controlSettings_fname);
    }

    static void zapData() {
        // This gets a bit tricky -- we can't just do SPIFFS.format because that would wipe out the mDNS name
        int i;

        if(doesFileExist(SPIFFS_controlConstants_fname)) SPIFFS.remove(SPIFFS_controlConstants_fname);
        if(doesFileExist(SPIFFS_controlSettings_fname)) SPIFFS.remove(SPIFFS_controlSettings_fname);

        char buf[20];
        for(i=0;i<MAX_SPIFFS_DEVICES;i++) {
            sprintf(buf, "%s%d", SPIFFS_device_fname_prepend, i);
            if(doesFileExist(buf)) SPIFFS.remove(buf);
        }
    }
};
