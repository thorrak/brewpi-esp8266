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
#include "PiLink.h"
#include "JsonKeys.h"

EepromManager eepromManager;
ESPEepromAccess eepromAccess;
ExtendedSettings extendedSettings;
UpstreamSettings upstreamSettings;


EepromManager::EepromManager()
{
	cache_loaded = false;
	// eepromSizeCheck();
}


bool EepromManager::initializeEeprom()
{
	// clear all eeprom
    eepromAccess.zapData();

	deviceManager.setupUnconfiguredDevices();

	// fetch the default values
	TempControl::loadDefaultConstants();
	TempControl::loadDefaultSettings();

	TempControl::storeConstants();
	TempControl::storeSettings();

		
	saveDefaultDevices();  // noop
	// set state to startup
	TempControl::init();

	return true;
}

uint8_t EepromManager::saveDefaultDevices() 
{
	// TODO - Determine if we need to do something here
	return 0;
}


bool EepromManager::applySettings()
{

	// start from a clean state		
	deviceManager.setupUnconfiguredDevices();
		
	// load the one chamber and one beer for now
	TempControl::loadConstants();
	TempControl::loadSettings();
	minTimes.loadFromSpiffs();
	
	
	for (uint8_t index = 0; index<Config::EepromFormat::MAX_DEVICES; index++)
	{	
		DeviceConfig deviceConfig = fetchDevice(index);
		if (deviceManager.isDeviceValid(deviceConfig, deviceConfig, index)) {
			deviceManager.installDevice(deviceConfig);
		} else {
			// The loaded device is not valid, so we need to reset it to defaults
			deviceConfig.setDefaults();
			eepromManager.storeDevice(deviceConfig, index);
		}			
	}
	return true;
}

void EepromManager::loadDevicesToCache() {
	for (uint8_t index = 0; index<Config::EepromFormat::MAX_DEVICES; index++) {	
		cached_devices[index].loadFromSpiffs(index);
	}
	cache_loaded = true;
}

DeviceConfig EepromManager::fetchDevice(uint8_t deviceIndex)
{
	if (!cache_loaded) {
		loadDevicesToCache();
	}

	if(deviceIndex<Config::EepromFormat::MAX_DEVICES) {
		// Always fetch the device from the cache when called
		return cached_devices[deviceIndex];
	} else {
		// This shouldn't ever get called, but sticking it here just in case
		DeviceConfig config;
		config.setDefaults();
		return config;
	}
	// eepromAccess.readDeviceDefinition(config, deviceIndex, sizeof(DeviceConfig));
}	


void EepromManager::storeDevice(DeviceConfig& config, uint8_t deviceIndex)
{
	if (deviceIndex<Config::EepromFormat::MAX_DEVICES) {
		config.storeToSpiffs(deviceIndex);
		cached_devices[deviceIndex].loadFromSpiffs(deviceIndex);
	}
}

void EepromManager::deleteDeviceWithFunction(DeviceFunction deviceFunction)
{
	if(deviceFunction == DEVICE_NONE) return; // Deleting a none device is effectively a noop -- just implement it as such here to save cycles
	
	for (uint8_t index = 0; index<Config::EepromFormat::MAX_DEVICES; index++) {	
		DeviceConfig deviceConfig = fetchDevice(index);
		if (deviceConfig.deviceFunction == deviceFunction) {
			deviceConfig.setDefaults();
			storeDevice(deviceConfig, index);
		}
	}
}


// Not sure if I should put this in EepromManager or ESPEepromAccess. Oh well.
// TODO - Make a decision & stick with it
#ifdef ESP8266_WiFi
String EepromManager::fetchmDNSName()
{
	String mdns_id;
	// The below loads the mDNS name from the file we saved it to (if the file exists)

    if (FILESYSTEM.exists("/mdns.txt")) {
        // The file exists - load it up
        File dns_name_file = FILESYSTEM.open("/mdns.txt", "r");  //TODO - Break "mdns.txt" into something configurable

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
    snprintf(ssid,15,"%04X",chip);

    mdns_id = "ESP" + (String) ssid;
#else
#error "Invalid device selected!"
#endif
	
	return mdns_id;
}

void EepromManager::savemDNSName(String mdns_id)
{
	File dns_name_file = FILESYSTEM.open("/mdns.txt", "w");
	if (dns_name_file) {
		// If the above fails, we weren't able to open the file for writing
		mdns_id.trim();
		dns_name_file.println(mdns_id);
	}
	dns_name_file.close();
}

#endif
