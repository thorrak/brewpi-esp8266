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
#include "TempSensor.h"
#include "PiLink.h"
#include "Ticks.h"


/**
 * \brief Initialize temp sensor
 */
void TempSensor::init()
{
	logDebug("tempsensor::init - begin %d", failedReadCount);
	if (_sensor && _sensor->init() && (failedReadCount<0 || failedReadCount>60)) {
		temperature temp = _sensor->read();
		if (temp!=TEMP_SENSOR_DISCONNECTED) {
			logDebug("initializing filters with value %d", temp);
			fastFilter.init(temp);
			slowFilter.init(temp);
			slopeFilter.init(0);
			prevOutputForSlope = slowFilter.readOutputDoublePrecision();
			failedReadCount = 0;
		}		
	}
}

/**
 * \brief Read the sensor and update the filters
 */
void TempSensor::update()
{
	temperature temp;
	if (!_sensor || (temp=_sensor->read())==TEMP_SENSOR_DISCONNECTED) {
		if(failedReadCount >= 0)
			failedReadCount++;
		failedReadCount = min(failedReadCount,int8_t(127));	// limit
		return;
	}
		
	fastFilter.add(temp);
	slowFilter.add(temp);
		
	// update slope filter every 3 samples.
	// averaged differences will give the slope. Use the slow filter as input
	updateCounter--;
	// initialize first read for slope filter after (255-4) seconds. This prevents an influence for the startup inaccuracy.
	if(updateCounter == 4){
		// only happens once after startup.
		prevOutputForSlope = slowFilter.readOutputDoublePrecision();
	}
	if(updateCounter == 0){
		temperature_precise slowFilterOutput = slowFilter.readOutputDoublePrecision();
		temperature_precise diff =  slowFilterOutput - prevOutputForSlope;
		temperature diff_upper = diff >> 16;
		if(diff_upper > 27){ // limit to prevent overflow INT_MAX/1200 = 27.14
			diff = (27l << 16);
		}
		else if(diff_upper < -27){
			diff = (-27l << 16);
		}
		slopeFilter.addDoublePrecision(1200*diff); // Multiply by 1200 (1h/4s), shift to single precision
		prevOutputForSlope = slowFilterOutput;
		updateCounter = 3;
	}
}

/**
 * \brief Read the sensor value after processing through the fast filter
 */
temperature TempSensor::readFastFiltered(){
	return fastFilter.readOutput(); //return most recent unfiltered value
}

/**
 * \brief Read the sensor value after processing through the slow filter
 */
temperature TempSensor::readSlowFiltered(){
  return slowFilter.readOutput(); //return most recent unfiltered value
}

/**
 * \brief Read the sensor value after processing through the slope filter
 */
temperature TempSensor::readSlope(){
	// return slope per hour.
	temperature_precise doublePrecision = slopeFilter.readOutputDoublePrecision();
	return doublePrecision>>16; // shift to single precision
}


/**
 * \brief Detect the positive peak
 *
 * Uses the detectPosPeak() method of the slow filter
 */
temperature TempSensor::detectPosPeak(){
	return slowFilter.detectPosPeak();
}


/**
 * \brief Detect the negative peak
 *
 * Uses the detectNegPeak() method of the slow filter
 */
temperature TempSensor::detectNegPeak(){
	return slowFilter.detectNegPeak();
}

/**
 * \brief Set the b coefficients on the fast filter
 *
 * @param b - New coefficient value
 */
void TempSensor::setFastFilterCoefficients(uint8_t b){
	fastFilter.setCoefficients(b);
}


/**
 * \brief Set the b coefficients on the slow filter
 *
 * @param b - New coefficient value
 */
void TempSensor::setSlowFilterCoefficients(uint8_t b){
	slowFilter.setCoefficients(b);
}


/**
 * \brief Set the b coefficients on the slope filter
 *
 * @param b - New coefficient value
 */
void TempSensor::setSlopeFilterCoefficients(uint8_t b){
	slopeFilter.setCoefficients(b);
}

/**
 * \brief Get wrapped sensor
 */
BasicTempSensor& TempSensor::sensor() {
	return *_sensor;
}
