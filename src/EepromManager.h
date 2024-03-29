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

#pragma once

#include "Brewpi.h"

#include "ESPEepromAccess.h"
#include "EepromStructs.h"


class DeviceConfig;


// todo - the Eeprom manager should avoid too frequent saves to the eeprom since it supports 100,000 writes. 
class EepromManager {
public:		
		
	EepromManager();

	/**
	 * Prepare the eeprom to accept device definitions. For RevA boards, the eeprom is populated with devices for
	 * beer/fridge temp sensor, and heating,cooling actuators and door switch.
	 */
	static bool initializeEeprom();

	/**
	 * Applies the settings from the eeprom
	 */
	bool applySettings();


	DeviceConfig fetchDevice(uint8_t deviceIndex);
	void storeDevice(DeviceConfig& config, uint8_t deviceIndex);
	void deleteDeviceWithFunction(DeviceFunction function);
	
	static uint8_t saveDefaultDevices();

#ifdef ESP8266_WiFi
	static String fetchmDNSName();
	static void savemDNSName(String mdns_id);
#endif


private:
	bool cache_loaded = false;
	void loadDevicesToCache();
	DeviceConfig cached_devices[Config::EepromFormat::MAX_DEVICES];

};


extern EepromManager eepromManager;
extern ExtendedSettings extendedSettings;
extern UpstreamSettings upstreamSettings;
