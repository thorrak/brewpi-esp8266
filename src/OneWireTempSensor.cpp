/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
 * Copyright 2013 Matthew McGowan.
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
#include "OneWireTempSensor.h"
#include "DallasTempNG.h"
#include "OneWireDevices.h"
#include "PiLink.h"
#include "Ticks.h"
#include "TemperatureFormats.h"

OneWireTempSensor::~OneWireTempSensor(){
	delete sensor;
};

/**
 * \brief Initializes the temperature sensor.
 *
 * This method is called when the sensor is first created and also any time the
 * sensor reports it's disconnected.  If the result is TEMP_SENSOR_DISCONNECTED
 * then subsequent calls to read() will also return TEMP_SENSOR_DISCONNECTED.
 * Clients should attempt to re-initialize the sensor by calling init() again.
 */
bool OneWireTempSensor::init() {
	// save address and pinNr for log messages
	char addressString[17];
	printBytes(sensorAddress, 8, addressString);
	// TODO - fix the following to use the defined OneWire pin
	DEBUG_ONLY(uint8_t pinNr = oneWire->pinNr());

	bool success = false;

	if (sensor==NULL) {
		sensor = new DallasTemperature(oneWire);
		if (sensor==NULL) {
			logErrorString(ERROR_SRAM_SENSOR, addressString);
		}
	}
	
	logDebug("init onewire sensor");
	// This quickly tests if the sensor is connected and initializes the reset detection.
	// During the main TempControl loop, we don't want to spend many seconds
	// scanning each sensor since this brings things to a halt.
	if (sensor && initConnection(*sensor, sensorAddress) && requestConversion()) {
		logDebug("init onewire sensor - wait for conversion");
		waitForConversion();
		temperature temp = readAndConstrainTemp();
		DEBUG_ONLY(logInfoIntStringTemp(INFO_TEMP_SENSOR_INITIALIZED, pinNr, addressString, temp));
		success = temp!=TEMP_SENSOR_DISCONNECTED && requestConversion();
	}	
	setConnected(success);
	logDebug("init onewire sensor complete %d", success);
	return success;
}


/**
 * \brief Request sensor measurement
 *
 * Sends a request to the OneWire bus for the configured device address to
 * begin the process of taking a measurement.  The length of time reqiured for
 * a OneWire device to sample the temperature depend on the requested precision
 * and powered vs. parasite powered.
 *
 * @see waitForConversion()
 */
bool OneWireTempSensor::requestConversion() {
	bool ok = sensor->requestTemperaturesByAddress(sensorAddress);
	setConnected(ok);
	return ok;
}

/**
 * \brief Set sensor connection status
 */
void OneWireTempSensor::setConnected(bool connected) {
	if (this->connected==connected)
		return; // state is stays the same

	char addressString[17];
	printBytes(sensorAddress, 8, addressString);
	this->connected = connected;
	if(connected){
		logInfoIntString(INFO_TEMP_SENSOR_CONNECTED, oneWirePin, addressString);
//		logInfoIntString(INFO_TEMP_SENSOR_CONNECTED, this->oneWire->pinNr(), addressString);
	}
	else{
		logWarningIntString(WARNING_TEMP_SENSOR_DISCONNECTED, oneWirePin, addressString);
//		logWarningIntString(WARNING_TEMP_SENSOR_DISCONNECTED, this->oneWire->pinNr(), addressString);
	}
}

/**
 * \brief Read the value of the sensor
 *
 * @return TEMP_SENSOR_DISCONNECTED if sensor is not connected, constrained temp otherwise.
 * @see readAndConstrainTemp()
 */
temperature OneWireTempSensor::read(){
	if (!connected)
		return TEMP_SENSOR_DISCONNECTED;

	temperature temp = readAndConstrainTemp();
	requestConversion();
	return temp;
}


/**
 * \brief Reads the temperature.
 *
 * If successful, constrains the temp to the range of the temperature type
 * and updates lastRequestTime. If unsuccessful, leaves lastRequestTime alone
 * and returns TEMP_SENSOR_DISCONNECTED.
 */
temperature OneWireTempSensor::readAndConstrainTemp()
{
	// getTempRaw is the same as sensor.getTemp() but also checks for reset
	temperature temp = getTempRaw(*sensor, sensorAddress);
	if(temp == DEVICE_DISCONNECTED_RAW){
		setConnected(false);
		return TEMP_SENSOR_DISCONNECTED;
	}

	const uint8_t shift = TEMP_FIXED_POINT_BITS - sensorPrecision; // difference in precision between DS18B20 format and temperature adt
	temp = constrainTemp(temp+calibrationOffset+(C_OFFSET>>shift), ((int) MIN_TEMP)>>shift, ((int) MAX_TEMP)>>shift)<<shift;
	return temp;
}
