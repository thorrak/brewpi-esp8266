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

#include "Brewpi.h"
#include <stddef.h>

#include "EepromManager.h"
#include "TempControl.h"
#include "EepromFormat.h"
#include "PiLink.h"

EepromManager eepromManager;
EepromAccess eepromAccess;

#define pointerOffset(x) offsetof(EepromFormat, x)

EepromManager::EepromManager()
{
	eepromSizeCheck();
}


bool EepromManager::hasSettings()
{
	// TODO - Technically, this acts as a version check as well.
	// We're eliminating that by just returning if the settings exist
//	uint8_t version = eepromAccess.readByte(pointerOffset(version));
//	return (version==EEPROM_FORMAT_VERSION);
	return eepromAccess.hasSettings();
}

void EepromManager::zapEeprom()
{
	eepromAccess.zapData();
}


void EepromManager::initializeEeprom()
{
	// clear all eeprom
//	for (uint16_t offset=0; offset<EepromFormat::MAX_EEPROM_SIZE; offset++)
//		eepromAccess.writeByte(offset, 0);	
	zapEeprom();

	deviceManager.setupUnconfiguredDevices();

	// fetch the default values
	tempControl.loadDefaultConstants();
	tempControl.loadDefaultSettings();	
	
	// write the default constants 
/*	for (uint8_t c=0; c<EepromFormat::MAX_CHAMBERS; c++) {
		eptr_t pv = pointerOffset(chambers)+(c*sizeof(ChamberBlock)) ;
		tempControl.storeConstants(pv+offsetof(ChamberBlock, chamberSettings.cc));
		pv += offsetof(ChamberBlock, beer)+offsetof(BeerBlock, cs);
		for (uint8_t b=0; b<ChamberBlock::MAX_BEERS; b++) {
//			logDeveloper(PSTR("EepromManager - saving settings for beer %d at %d"), b, (uint16_t)pv);
			tempControl.storeSettings(pv);	
			pv += sizeof(BeerBlock);		// advance to next beer
		}
	}*/
	// TODO - Eventually, restore ability to have more than one chamber/beer
	tempControl.storeConstants(0); // Replacing 'pv' with 0 since we're no longer using EEPROM
	tempControl.storeSettings(0); // Replacing 'pv' with 0 since we're no longer using EEPROM

	// set the version flag - so that storeDevice will work
//	eepromAccess.writeByte(0, EEPROM_FORMAT_VERSION);  // We don't save the EEPROM version anywhere now
		
	saveDefaultDevices();  // noop
	// set state to startup
	tempControl.init();

}

uint8_t EepromManager::saveDefaultDevices() 
{
	return 0;
}

#define arraySize(x) (sizeof(x)/sizeof(x[0]))


bool EepromManager::applySettings()
{
    if (!hasSettings()) {
        return false;  // TODO - This is where the EEPROM reset code should be called.
    }

	// start from a clean state		
	deviceManager.setupUnconfiguredDevices();
		
	logDebug("Applying settings");

	// load the one chamber and one beer for now
	eptr_t pv = pointerOffset(chambers);
	tempControl.loadConstants(pv+offsetof(ChamberBlock, chamberSettings.cc));	
	tempControl.loadSettings(pv+offsetof(ChamberBlock, beer[0].cs));
	
	logDebug("Applied settings");
	
	
	DeviceConfig deviceConfig;
	for (uint8_t index = 0; fetchDevice(deviceConfig, index); index++)
	{	
		if (deviceManager.isDeviceValid(deviceConfig, deviceConfig, index))
			deviceManager.installDevice(deviceConfig);
		else {
			clear((uint8_t*)&deviceConfig, sizeof(deviceConfig));
			eepromManager.storeDevice(deviceConfig, index);
		}			
	}
	return true;
}

void EepromManager::storeTempConstantsAndSettings()
{
	uint8_t chamber = 0;
	eptr_t pv = pointerOffset(chambers);
	pv += sizeof(ChamberBlock)*chamber;
	tempControl.storeConstants(pv+offsetof(ChamberBlock, chamberSettings.cc));
		
	storeTempSettings();
}

void EepromManager::storeTempSettings()
{
	uint8_t chamber = 0;
	eptr_t pv = pointerOffset(chambers);
	pv += sizeof(ChamberBlock)*chamber;
	// for now assume just one beer. 
	tempControl.storeSettings(pv+offsetof(ChamberBlock, beer[0].cs));	
}

bool EepromManager::fetchDevice(DeviceConfig& config, int8_t deviceIndex)
{
	bool ok = (hasSettings() && deviceIndex<EepromFormat::MAX_DEVICES);
	if (ok)
		eepromAccess.readDeviceDefinition(config, deviceIndex, sizeof(DeviceConfig));
//        eepromAccess.readDeviceDefinition(config, pointerOffset(devices)+sizeof(DeviceConfig)*deviceIndex, sizeof(DeviceConfig));
	return ok;
}	

bool EepromManager::storeDevice(const DeviceConfig& config, int8_t deviceIndex)
{
	bool ok = (hasSettings() && deviceIndex<EepromFormat::MAX_DEVICES);
	if (ok)
        eepromAccess.writeDeviceDefinition(deviceIndex, config, sizeof(DeviceConfig));
//        eepromAccess.writeDeviceDefinition(pointerOffset(devices)+sizeof(DeviceConfig)*deviceIndex, config, sizeof(DeviceConfig));
	return ok;
}


// Not sure if I should put this in EepromManager or ESPEepromAccess. Oh well.
// TODO - Make a decision & stick with it
#ifdef ESP8266_WiFi
String EepromManager::fetchmDNSName()
{
	String mdns_id;
	// The below loads the mDNS name from the file we saved it to (if the file exists)
	if (SPIFFS.exists("/mdns.txt")) {
		// The file exists - load it up
		File dns_name_file = SPIFFS.open("/mdns.txt", "r");  //TODO - Break "mdns.txt" into something configurable
		if (dns_name_file) {
			// Assuming everything goes well, read in the mdns name
			mdns_id = dns_name_file.readStringUntil('\n');
			mdns_id.trim();
			return mdns_id;
		}
	}

#if defined(ESP8266)
    mdns_id = "ESP" + String(ESP.getChipId());
#elif defined(ESP32)
    // There isn't a straightforward "getChipId" function on an ESP32, so we'll have to make do
    char ssid[15]; //Create a Unique AP from MAC address
    uint64_t chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
    uint16_t chip = (uint16_t)(chipid>>32);
    snprintf(ssid,15,"ESP%04X",chip);

    mdns_id = "ESP" + (String) ssid;
#else
#error "Invalid device selected!"
#endif
	
	return mdns_id;
}

void EepromManager::savemDNSName(String mdns_id)
{
	File dns_name_file = SPIFFS.open("/mdns.txt", "w");
	if (dns_name_file) {
		// If the above fails, we weren't able to open the file for writing
		mdns_id.trim();
		dns_name_file.println(mdns_id);
	}
	dns_name_file.close();
}

#endif


void fill(int8_t* p, uint8_t size) {
	while (size-->0) *p++ = -1;
}
void clear(uint8_t* p, uint8_t size) {
	while (size-->0) *p++ = 0;
}

