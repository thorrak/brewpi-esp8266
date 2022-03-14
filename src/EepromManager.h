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


void fill(int8_t* p, uint8_t size);

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
	 * Determines if this eeprom has settings.
	 */
	static bool hasSettings();

	/**
	 * Applies the settings from the eeprom
	 */
	static bool applySettings();

	/**
	 * Save the chamber constants and beer settings to eeprom for the currently active chamber.
	 */
	static void storeTempConstantsAndSettings();

	/**
	 * Save just the beer temp settings.
	 */
	static void storeTempSettings();

	static DeviceConfig fetchDevice(uint8_t deviceIndex);
	static bool storeDevice(DeviceConfig& config, uint8_t deviceIndex);
	
	static uint8_t saveDefaultDevices();

#ifdef ESP8266_WiFi
	static String fetchmDNSName();
	static void savemDNSName(String mdns_id);
#endif

};


extern EepromManager eepromManager;
