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
//#include <Arduino.h>
#include <EEPROM.h>
#include "Logger.h" // Remove this once done
#include "EepromStructs.h"
#include "EepromFormat.h"


#define SPIFFS_controlConstants_fname "/controlConstants"
#define SPIFFS_controlSettings_fname "/controlSettings"
#define SPIFFS_device_fname_prepend "/dev"

#define MAX_SPIFFS_DEVICES EepromFormat::MAX_DEVICES    // This is a shortcut. I know it's a shortcut. I don't care that it's a shortcut.
                                // ... TODO - Fix this shortcut so that we can support future features


//TODO - Clean this up
class ESPEepromAccess
{
private:
	template <class T> static bool writeBlockToFile(String target_name, T& data) {
		if (SPIFFS.begin()) {  // This may be an issue if called multiple times - going to assume it's not
			File out_file = SPIFFS.open(target_name, "w");
			if (out_file) {
                Serial.println("Preparing to recast data");
//				out_file.write(reinterpret_cast<const uint8_t*>(&data), sizeof(T));
                out_file.write((const uint8_t*)&data, sizeof(data));
                Serial.println("Recast successful, preparing to write");
				out_file.close();
                Serial.println("Write successful");
                return true;
			} else {
                Serial.println("SPIFFS.open() failed");
            }
		} else {
            // There's some kind of issue with SPIFFS or something.
            // TODO - Log this
            Serial.println("SPIFFS.begin() failed");
        }
		return false;
	}

	template <class T> static bool readBlockFromFile(String target_name, T& data) {
		if (SPIFFS.begin()) {  // This may be an issue if called multiple times - going to assume it's not
			File in_file = SPIFFS.open(target_name, "r");
			if (in_file) {
                uint8_t holding[sizeof(T)];
				in_file.read(holding, sizeof(data));
                memcpy(holding, &data, sizeof(data));
				in_file.close();
            }
        }
		// There's some kind of issue with SPIFFS or something.
		// TODO - Log this
		return false;
	}

    static bool doesFileExist(String target_name) {
        if (SPIFFS.begin()) {  // This may be an issue if called multiple times - going to assume it's not
            return SPIFFS.exists(target_name);
        }
        // There's some kind of issue with SPIFFS or something.
        // TODO - Log this
        return false;
    }


public:
	// Since we're basically switching to using SPIFFS for everything, I don't want these to compile
/*	static uint8_t readByte(eptr_t offset) {
		return EEPROM.read(offset);
	}
	static void writeByte(eptr_t offset, uint8_t value) {
		EEPROM.write(offset, value);
	}*/

	static void readControlSettings(ControlSettings& target, eptr_t offset, uint16_t size) {
        Serial.println("readControlSettings called");
        readBlockFromFile(SPIFFS_controlSettings_fname, target);
	}

	static void readControlConstants(ControlConstants& target, eptr_t offset, uint16_t size) {
        Serial.println("readControlConstants called");
        readBlockFromFile(SPIFFS_controlConstants_fname, target);
	}

	static void readDeviceDefinition(DeviceConfig& target, eptr_t deviceID, uint16_t size) {
        Serial.println("readDeviceDefinition called");
        readBlockFromFile(SPIFFS_device_fname_prepend + deviceID, target);  // deviceID was previously an offset in memory - now it's a sequential #
	}

	static void writeControlSettings(eptr_t target, ControlSettings& source, uint16_t size) {
        Serial.println("writeControlSettings called");
		writeBlockToFile(SPIFFS_controlSettings_fname, source);
	}

	static void writeControlConstants(eptr_t target, ControlConstants& source, uint16_t size) {
        Serial.println("writeControlConstants called");
        writeBlockToFile(SPIFFS_controlConstants_fname, source);
	}

	static void writeDeviceDefinition(eptr_t deviceID, const DeviceConfig& source, uint16_t size) {
        Serial.println("writeDeviceDefinition called");
        writeBlockToFile(SPIFFS_device_fname_prepend + deviceID, source);  // deviceID was previously an offset in memory - now it's a sequential #
		logWarningIntString(0, sizeof(source), "writeDeviceDefinition called");
	}

    static bool hasSettings() {
        return doesFileExist(SPIFFS_controlSettings_fname);
    }

    static void zapData() {
        Serial.println("zapData called");
        // This gets a bit tricky -- we can't just do SPIFFS.format because that would wipe out the mDNS name
        int i;

        if(doesFileExist(SPIFFS_controlConstants_fname)) SPIFFS.remove(SPIFFS_controlConstants_fname);
        if(doesFileExist(SPIFFS_controlSettings_fname)) SPIFFS.remove(SPIFFS_controlSettings_fname);

        for(i=0;i<MAX_SPIFFS_DEVICES;i++) {
            if(doesFileExist(SPIFFS_device_fname_prepend + i)) SPIFFS.remove(SPIFFS_device_fname_prepend + i);
        }
    }

	static void commit(void) {
//		EEPROM.commit();
	}

	static void set_manual_commit(const bool status) {
//		manual_commit = status;
	}

};
