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

#include "Pins.h"
#include <limits.h>

#include "TemperatureFormats.h"
#include "TempControl.h"
#include "PiLink.h"
#include "TempSensor.h"
#include "Ticks.h"
#include "TempSensorMock.h"
#include "EepromManager.h"
#include "TempSensorDisconnected.h"
#include "RotaryEncoder.h"

TempControl tempControl;
MinTimes minTimes;

#if TEMP_CONTROL_STATIC

extern ValueSensor<bool> defaultSensor;
extern ValueActuator defaultActuator;
extern DisconnectedTempSensor defaultTempSensor;

// These sensors are switched out to implement multi-chamber.
TempSensor* TempControl::beerSensor;
TempSensor* TempControl::fridgeSensor;
BasicTempSensor* TempControl::ambientSensor = &defaultTempSensor;


Actuator* TempControl::heater = &defaultActuator;
Actuator* TempControl::cooler = &defaultActuator;
Actuator* TempControl::light = &defaultActuator;
Actuator* TempControl::fan = &defaultActuator;

ValueActuator cameraLightState;		
AutoOffActuator TempControl::cameraLight(600, &cameraLightState);	// timeout 10 min
Sensor<bool>* TempControl::door = &defaultSensor;
	
// Control parameters
ControlConstants TempControl::cc;
ControlSettings TempControl::cs;
ControlVariables TempControl::cv;
	
	// State variables
uint8_t TempControl::state;
bool TempControl::doPosPeakDetect;
bool TempControl::doNegPeakDetect;
bool TempControl::doorOpen;
	
	// keep track of beer setting stored in EEPROM
temperature TempControl::storedBeerSetting;
	
	// Timers
uint16_t TempControl::lastIdleTime;
uint16_t TempControl::lastHeatTime;
uint16_t TempControl::lastCoolTime;
uint16_t TempControl::waitTime;
#endif


#ifndef min
#define min _min
#endif

#ifndef max
#define max _max
#endif


/**
 * Initialize the temp control system.  Done at startup.
 */
void TempControl::init(){
	state=IDLE;
	cs.mode = Modes::off;

	minTimes.setDefaults();  // Update the min times before we initialize temp control

	cameraLight.setActive(false);

	// this is for cases where the device manager hasn't configured beer/fridge sensor.	
	if (beerSensor==NULL) {
		beerSensor = new TempSensor(TEMP_SENSOR_TYPE_BEER, &defaultTempSensor);
		beerSensor->init();
	}
		
	if (fridgeSensor==NULL) {
		fridgeSensor = new TempSensor(TEMP_SENSOR_TYPE_FRIDGE, &defaultTempSensor);
		fridgeSensor->init();
	}
	
	updateTemperatures();
	reset();

	// Do not allow heating/cooling directly after reset.
	// A failing script + CRON + Arduino uno (which resets on serial connect) could damage the compressor
	// For test purposes, set these to -3600 to eliminate waiting after reset
	lastHeatTime = 0;
	lastCoolTime = 0;
}


/**
 * Reset the peak detect flags
 */
void TempControl::reset(){
	doPosPeakDetect=false;
	doNegPeakDetect=false;
}


/**
 * Get an update from a sensor.
 *
 * @param sensor - Sensor to check
 */
void updateSensor(TempSensor* sensor) {
	sensor->update();
	if(!sensor->isConnected()) {
		sensor->init();
	}
}

/**
 * Update all installed temp sensors.
 *
 * This updates beer, fridge & room sensors.
 */
void TempControl::updateTemperatures(){
	
	updateSensor(beerSensor);
	updateSensor(fridgeSensor);
	
	// Read ambient sensor to keep the value up to date. If no sensor is connected, this does nothing.
	// This prevents a delay in serial response because the value is not up to date.
	if(ambientSensor->read() == TEMP_SENSOR_DISCONNECTED){
		ambientSensor->init(); // try to reconnect a disconnected, but installed sensor
	}
}

void TempControl::updatePID(){
	static unsigned char integralUpdateCounter = 0;
	if(tempControl.modeIsBeer()){
		if(cs.beerSetting == INVALID_TEMP){
			// beer setting is not updated yet
			// set fridge to unknown too
			cs.fridgeSetting = INVALID_TEMP;
			return;
		}
		
		// fridge setting is calculated with PID algorithm. Beer temperature error is input to PID
		cv.beerDiff =  cs.beerSetting - beerSensor->readSlowFiltered();
		cv.beerSlope = beerSensor->readSlope();
		temperature fridgeFastFiltered = fridgeSensor->readFastFiltered();
			
		if(integralUpdateCounter++ == 60){
			integralUpdateCounter = 0;
			
			temperature integratorUpdate = cv.beerDiff;
			
			// Only update integrator in IDLE, because thats when the fridge temp has reached the fridge setting.
			// If the beer temp is still not correct, the fridge setting is too low/high and integrator action is needed.
			if(state != IDLE){
				integratorUpdate = 0;
			}
			else if(abs(integratorUpdate) < cc.iMaxError){
				// difference is smaller than iMaxError				
				// check additional conditions to see if integrator should be active to prevent windup
				bool updateSign = (integratorUpdate > 0); // 1 = positive, 0 = negative
				bool integratorSign = (cv.diffIntegral > 0);		
				
				if(updateSign == integratorSign){
					// beerDiff and integrator have same sign. Integrator would be increased.
					
					// If actuator is already at max increasing actuator will only cause integrator windup.
					integratorUpdate = (cs.fridgeSetting >= cc.tempSettingMax) ? 0 : integratorUpdate;
					integratorUpdate = (cs.fridgeSetting <= cc.tempSettingMin) ? 0 : integratorUpdate;
					integratorUpdate = ((cs.fridgeSetting - cs.beerSetting) >= cc.pidMax) ? 0 : integratorUpdate;
					integratorUpdate = ((cs.beerSetting - cs.fridgeSetting) >= cc.pidMax) ? 0 : integratorUpdate;
										
					// cooling and fridge temp is more than 2 degrees from setting, actuator is saturated.
					integratorUpdate = (!updateSign && (fridgeFastFiltered > (cs.fridgeSetting +1024))) ? 0 : integratorUpdate;
					
					// heating and fridge temp is more than 2 degrees from setting, actuator is saturated.
					integratorUpdate = (updateSign && (fridgeFastFiltered < (cs.fridgeSetting -1024))) ? 0 : integratorUpdate;
				}
				else{
					// integrator action is decreased. Decrease faster than increase.
					integratorUpdate = integratorUpdate*2;
				}	
			}
			else{
				// decrease integral by 1/8 when far from the end value to reset the integrator
				integratorUpdate = -(cv.diffIntegral >> 3);		
			}
			cv.diffIntegral = cv.diffIntegral + integratorUpdate;
		}			
		
		// calculate PID parts. Use long_temperature to prevent overflow
		cv.p = multiplyFactorTemperatureDiff(cc.Kp, cv.beerDiff);
		cv.i = multiplyFactorTemperatureDiffLong(cc.Ki, cv.diffIntegral);
		cv.d = multiplyFactorTemperatureDiff(cc.Kd, cv.beerSlope);
		long_temperature newFridgeSetting = cs.beerSetting;
		newFridgeSetting += cv.p;
		newFridgeSetting += cv.i;
		newFridgeSetting += cv.d;
		
		// constrain to tempSettingMin or beerSetting - pidMAx, whichever is lower.
		temperature lowerBound = (cs.beerSetting <= cc.tempSettingMin + cc.pidMax) ? cc.tempSettingMin : cs.beerSetting - cc.pidMax;
		// constrain to tempSettingMax or beerSetting + pidMAx, whichever is higher.
		temperature upperBound = (cs.beerSetting >= cc.tempSettingMax - cc.pidMax) ? cc.tempSettingMax : cs.beerSetting + cc.pidMax;
		
		cs.fridgeSetting = constrain(constrainTemp16(newFridgeSetting), lowerBound, upperBound);
	}
	else if(cs.mode == Modes::fridgeConstant){
		// FridgeTemperature is set manually, use INVALID_TEMP to indicate beer temp is not active
		cs.beerSetting = INVALID_TEMP;
	}
}

void TempControl::updateState(){
	//update state
	bool stayIdle = false;
	bool newDoorOpen = door->sense();
		
	if(newDoorOpen!=doorOpen) {
		doorOpen = newDoorOpen;
		String annotation = "";
		annotation += "Fridge door ";
		annotation += doorOpen ? "opened" : "closed";
		piLink.printTemperatures(0, annotation.c_str());
	}

	if(cs.mode == Modes::off){
		state = STATE_OFF;
		stayIdle = true;
	} else if( cs.fridgeSetting == INVALID_TEMP || !fridgeSensor->isConnected() || (!beerSensor->isConnected() && tempControl.modeIsBeer())){
        // stay idle when one of the required sensors is disconnected, or the fridge setting is INVALID_TEMP
        // Of note - setting the mode to Modes::off also sets cs.fridgeSetting to INVALID_TEMP
		state = IDLE;
		stayIdle = true;
	}
	
	uint16_t sinceIdle = timeSinceIdle();
	uint16_t sinceCooling = timeSinceCooling();
	uint16_t sinceHeating = timeSinceHeating();
	temperature fridgeFast = fridgeSensor->readFastFiltered();
	temperature beerFast = beerSensor->readFastFiltered();
	ticks_seconds_t secs = ticks.seconds();
	switch(state)
	{
		case IDLE:
		case STATE_OFF:
		case WAITING_TO_COOL:
		case WAITING_TO_HEAT:
		case WAITING_FOR_PEAK_DETECT:
		{
			lastIdleTime=secs;		
			// set waitTime to zero. It will be set to the maximum required waitTime below when wait is in effect.
			if(stayIdle){
				break;
			}
			resetWaitTime();
			if(fridgeFast > (cs.fridgeSetting+cc.idleRangeHigh) ){  // fridge temperature is too high			
				tempControl.updateWaitTime(minTimes.MIN_SWITCH_TIME, sinceHeating);			
				if(cs.mode==Modes::fridgeConstant){
					tempControl.updateWaitTime(minTimes.MIN_COOL_OFF_TIME_FRIDGE_CONSTANT, sinceCooling);
				}
				else{
					if(beerFast < (cs.beerSetting + 16) ){ // If beer is already under target, stay/go to idle. 1/2 sensor bit idle zone
						state = IDLE; // beer is already colder than setting, stay in or go to idle
						break;
					}
					tempControl.updateWaitTime(minTimes.MIN_COOL_OFF_TIME, sinceCooling);
				}
				if(tempControl.cooler != &defaultActuator){
					if(getWaitTime() > 0){
						state = WAITING_TO_COOL;
					}
					else{
						state = COOLING;	
					}
				}
			}
			else if(fridgeFast < (cs.fridgeSetting+cc.idleRangeLow)){  // fridge temperature is too low
				tempControl.updateWaitTime(minTimes.MIN_SWITCH_TIME, sinceCooling);
				tempControl.updateWaitTime(minTimes.MIN_HEAT_OFF_TIME, sinceHeating);
				if(cs.mode!=Modes::fridgeConstant){
					if(beerFast > (cs.beerSetting - 16)){ // If beer is already over target, stay/go to idle. 1/2 sensor bit idle zone
						state = IDLE;  // beer is already warmer than setting, stay in or go to idle
						break;
					}
				}
				if(tempControl.heater != &defaultActuator || (cc.lightAsHeater && (tempControl.light != &defaultActuator))){
					if(getWaitTime() > 0){
						state = WAITING_TO_HEAT;
					}
					else{
						state = HEATING;
					}
				}
			}
			else{
				state = IDLE; // within IDLE range, always go to IDLE
				break;
			}
			if(state == HEATING || state == COOLING){	
				if(doNegPeakDetect == true || doPosPeakDetect == true){
					// If peak detect is not finished, but the fridge wants to switch to heat/cool
					// Wait for peak detection and display 'Await peak detect' on display
					state = WAITING_FOR_PEAK_DETECT;
					break;
				}
			}
		}			
		break; 
		case COOLING:
		case COOLING_MIN_TIME:
		{
			doNegPeakDetect=true;
			lastCoolTime = secs;
			updateEstimatedPeak(cc.maxCoolTimeForEstimate, cs.coolEstimator, sinceIdle);
			state = COOLING; // set to cooling here, so the display of COOLING/COOLING_MIN_TIME is correct
			
			// stop cooling when estimated fridge temp peak lands on target or if beer is already too cold (1/2 sensor bit idle zone)
			if(cv.estimatedPeak <= cs.fridgeSetting || (cs.mode != Modes::fridgeConstant && beerFast < (cs.beerSetting - 16))){
				if(sinceIdle > minTimes.MIN_COOL_ON_TIME){
					cv.negPeakEstimate = cv.estimatedPeak; // remember estimated peak when I switch to IDLE, to adjust estimator later
					state=IDLE;
					break;
				}
				else{
					state = COOLING_MIN_TIME;
					break;
				}				
			}
		}
		break;
		case HEATING:
		case HEATING_MIN_TIME:
		{
			doPosPeakDetect=true;
			lastHeatTime=secs;
			updateEstimatedPeak(cc.maxHeatTimeForEstimate, cs.heatEstimator, sinceIdle);
			state = HEATING; // reset to heating here, so the display of HEATING/HEATING_MIN_TIME is correct
			
			// stop heating when estimated fridge temp peak lands on target or if beer is already too warm (1/2 sensor bit idle zone)
			if(cv.estimatedPeak >= cs.fridgeSetting || (cs.mode != Modes::fridgeConstant && beerFast > (cs.beerSetting + 16))){
				if(sinceIdle > minTimes.MIN_HEAT_ON_TIME){
					cv.posPeakEstimate=cv.estimatedPeak; // remember estimated peak when I switch to IDLE, to adjust estimator later
					state=IDLE;
					break;
				}
				else{
					state = HEATING_MIN_TIME;
					break;
				}
			}
		}
		break;
	}			
}

void TempControl::updateEstimatedPeak(uint16_t timeLimit, temperature estimator, uint16_t sinceIdle)
{
	uint16_t activeTime = min(timeLimit, sinceIdle); // heat or cool time in seconds
	temperature estimatedOvershoot = ((long_temperature) estimator * activeTime)/3600; // overshoot estimator is in overshoot per hour
	if(stateIsCooling()){
		estimatedOvershoot = -estimatedOvershoot; // when cooling subtract overshoot from fridge temperature
	}
	cv.estimatedPeak = fridgeSensor->readFastFiltered() + estimatedOvershoot;		
}

void TempControl::updateOutputs() {
	if (cs.mode==Modes::test)
		return;
		
	cameraLight.update();
	bool heating = stateIsHeating();
	bool cooling = stateIsCooling();
	cooler->setActive(cooling);		
	heater->setActive(!cc.lightAsHeater && heating);	
	light->setActive(isDoorOpen() || (cc.lightAsHeater && heating) || cameraLightState.isActive());	
	fan->setActive(heating || cooling);
}


void TempControl::detectPeaks(){  
	//detect peaks in fridge temperature to tune overshoot estimators
	LOG_ID_TYPE detected = 0;
	temperature peak, estimate, error, oldEstimator, newEstimator;
	
	if(doPosPeakDetect && !stateIsHeating()){
		peak = fridgeSensor->detectPosPeak();
		estimate = cv.posPeakEstimate;
		error = peak-estimate;
		oldEstimator = cs.heatEstimator;
		if(peak != INVALID_TEMP){
			// positive peak detected
			if(error > cc.heatingTargetUpper){
				// Peak temperature was higher than the estimate.
				// Overshoot was higher than expected
				// Increase estimator to increase the estimated overshoot
				increaseEstimator(&(cs.heatEstimator), error);
			}
			if(error < cc.heatingTargetLower){
				// Peak temperature was lower than the estimate.
				// Overshoot was lower than expected
				// Decrease estimator to decrease the estimated overshoot
				decreaseEstimator(&(cs.heatEstimator), error);
			}
			detected = INFO_POSITIVE_PEAK;
		}
		else if(timeSinceHeating() > minTimes.HEAT_PEAK_DETECT_TIME){
			if(fridgeSensor->readFastFiltered() < (cv.posPeakEstimate+cc.heatingTargetLower)){
				// Idle period almost reaches maximum allowed time for peak detection
				// This is the heat, then drift up too slow (but in the right direction).
				// estimator is too high
				peak=fridgeSensor->readFastFiltered();
				decreaseEstimator(&(cs.heatEstimator), error);			
				detected = INFO_POSITIVE_DRIFT;
			}
			else{
				// maximum time for peak estimation reached
				doPosPeakDetect = false;	
			}
		}
		if(detected){
			newEstimator = cs.heatEstimator;	
			cv.posPeak = peak;
			doPosPeakDetect = false;
		}
	}			
	else if(doNegPeakDetect && !stateIsCooling()){
		peak = fridgeSensor->detectNegPeak();
		estimate = cv.negPeakEstimate;
		error = peak-estimate;
		oldEstimator = cs.coolEstimator;
		if(peak != INVALID_TEMP){
			// negative peak detected
			if(error < cc.coolingTargetLower){
				// Peak temperature was lower than the estimate.
				// Overshoot was higher than expected
				// Increase estimator to increase the estimated overshoot
				increaseEstimator(&(cs.coolEstimator), error);
			}
			if(error > cc.coolingTargetUpper){
				// Peak temperature was higher than the estimate.
				// Overshoot was lower than expected
				// Decrease estimator to decrease the estimated overshoot
				decreaseEstimator(&(cs.coolEstimator), error);

			}
			detected = INFO_NEGATIVE_PEAK;
		}
		else if(timeSinceCooling() > minTimes.COOL_PEAK_DETECT_TIME){
			if(fridgeSensor->readFastFiltered() > (cv.negPeakEstimate+cc.coolingTargetUpper)){
				// Idle period almost reaches maximum allowed time for peak detection
				// This is the cooling, then drift down too slow (but in the right direction).
				// estimator is too high
				peak = fridgeSensor->readFastFiltered();
				decreaseEstimator(&(cs.coolEstimator), error);
				detected = INFO_NEGATIVE_DRIFT;
			}
			else{
				// maximum time for peak estimation reached
				doNegPeakDetect=false;
			}
		}
		if(detected){
			newEstimator = cs.coolEstimator;
			cv.negPeak = peak;
			doNegPeakDetect=false;
		}
	}
	if(detected){
		// send out log message for type of peak detected
		logInfoTempTempFixedFixed(detected, peak, estimate, oldEstimator, newEstimator);
	}
}

/**
 * Increase the estimator value.
 *
 * Increase estimator at least 20%, max 50%s
 */
void TempControl::increaseEstimator(temperature * estimator, temperature error){
	temperature factor = 614 + constrainTemp((temperature) abs(error)>>5, 0, 154); // 1.2 + 3.1% of error, limit between 1.2 and 1.5
	*estimator = multiplyFactorTemperatureDiff(factor, *estimator);
	if(*estimator < 25){
		*estimator = intToTempDiff(5)/100; // make estimator at least 0.05
	}
	TempControl::storeSettings();
}

/**
 * Decrease the esimator value.
 *
 * Decrease estimator at least 16.7% (1/1.2), max 33.3% (1/1.5)
 */
void TempControl::decreaseEstimator(temperature * estimator, temperature error){
	temperature factor = 426 - constrainTemp((temperature) abs(error)>>5, 0, 85); // 0.833 - 3.1% of error, limit between 0.667 and 0.833
	*estimator = multiplyFactorTemperatureDiff(factor, *estimator);
	TempControl::storeSettings();
}

/**
 * Get time since the cooler was last ran
 */
uint16_t TempControl::timeSinceCooling(){
	return ticks.timeSince(lastCoolTime);
}

/**
 * Get time since the heater was last ran
 */
uint16_t TempControl::timeSinceHeating(){
	return ticks.timeSince(lastHeatTime);
}

/**
 * Get time that the controller has been neither cooling nor heating
 */
uint16_t TempControl::timeSinceIdle(){
	return ticks.timeSince(lastIdleTime);
}

/**
 * Load default settings
 */
void TempControl::loadDefaultSettings(){
    cs.setDefaults();
#if BREWPI_EMULATE
	setMode(Modes::beerConstant);
#else	
	setMode(Modes::off);
#endif	
}

/**
 * Store control constants to EEPROM.
 */
void TempControl::storeConstants() {
    // Now that control constants are an object, use that for loading/saving
    cc.storeToSpiffs();
}

/**
 * Load control constants from EEPROM
 */
void TempControl::loadConstants(){
  // Now that control constants are an object, use that for loading/saving
  cc.loadFromSpiffs();
  initFilters();
}


/**
 * Write new settings to EEPROM to be able to reload them after a reset
 * The update functions only write to EEPROM if the value has changed
 */
void TempControl::storeSettings(){
	cs.storeToSpiffs();
	storedBeerSetting = cs.beerSetting;
}

/**
 * Read settings from EEPROM
 */
void TempControl::loadSettings(){
  cs.loadFromSpiffs();
	logDebug("loaded settings");
	storedBeerSetting = cs.beerSetting;
	setMode(cs.mode, true);		// force the mode update
}

/**
 * Load default control constants
 */
void TempControl::loadDefaultConstants(){
  // Rather than using memcpy to copy over a default struct of settings, use the class method
  // (We have the flash space to do this the less flash-conscious way)
  cc.setDefaults();
	initFilters();
}

/**
 * Initialize the fridge & beer sensor filter coefficients
 *
 * @see CascadedFilter
 */
void TempControl::initFilters()
{
	fridgeSensor->setFastFilterCoefficients(cc.fridgeFastFilter);
	fridgeSensor->setSlowFilterCoefficients(cc.fridgeSlowFilter);
	fridgeSensor->setSlopeFilterCoefficients(cc.fridgeSlopeFilter);
	beerSensor->setFastFilterCoefficients(cc.beerFastFilter);
	beerSensor->setSlowFilterCoefficients(cc.beerSlowFilter);
	beerSensor->setSlopeFilterCoefficients(cc.beerSlopeFilter);		
}


/**
 * Set control mode
 *
 * @param newMode - New control mode
 * @param force - Set the mode & reset control state, even if controler is already in the requested mode
 */
void TempControl::setMode(char newMode, bool force){
	logDebug("TempControl::setMode from %c to %c", cs.mode, newMode);
	
	if(newMode != cs.mode || state == WAITING_TO_HEAT || state == WAITING_TO_COOL || state == WAITING_FOR_PEAK_DETECT){
		state = IDLE;
		force = true;
	}
	if (force) {
		cs.mode = newMode;
		if(newMode == Modes::off){
			cs.beerSetting = INVALID_TEMP;
			cs.fridgeSetting = INVALID_TEMP;
		}
		TempControl::storeSettings();
	}
}


/**
 * Get current beer temperature
 */
temperature TempControl::getBeerTemp(){
	if(beerSensor->isConnected()){
		return beerSensor->readFastFiltered();
	}
	else{
		return INVALID_TEMP;
	}
}

/**
 * Get current beer target temperature
 */
temperature TempControl::getBeerSetting(){
	return cs.beerSetting;
}


/**
 * Get current fridge temperature
 */
temperature TempControl::getFridgeTemp(){
	if(fridgeSensor->isConnected()){
		return fridgeSensor->readFastFiltered();
	} else {
		return INVALID_TEMP;
	}
}

/**
 * Get current fridge target temperature
 */
temperature TempControl::getFridgeSetting(){
	return cs.fridgeSetting;
}


/**
 * Set desired beer temperature
 *
 * @param newTemp - new target temperature
 */
void TempControl::setBeerTemp(temperature newTemp){
	temperature oldBeerSetting = cs.beerSetting;
	cs.beerSetting= newTemp;
	if(abs(oldBeerSetting - newTemp) > intToTempDiff(1)/2){ // more than half degree C difference with old setting
		reset(); // reset controller
	}
	updatePID();
	updateState();
	if(cs.mode != Modes::beerProfile || abs(storedBeerSetting - newTemp) > intToTempDiff(1)/4){
		// more than 1/4 degree C difference with EEPROM
		// Do not store settings every time in profile mode, because EEPROM has limited number of write cycles.
		// A temperature ramp would cause a lot of writes
		// If Raspberry Pi is connected, it will update the settings anyway. This is just a safety feature.
		TempControl::storeSettings();
	}
}

/**
 * Set desired fridge temperature
 *
 * @param newTemp - New target temperature
 */
void TempControl::setFridgeTemp(temperature newTemp){
	cs.fridgeSetting = newTemp;
	reset(); // reset peak detection and PID
	updatePID();
	updateState();
	TempControl::storeSettings();
}

/**
 * Check if current state is cooling (or waiting to cool)
 */
bool TempControl::stateIsCooling(){
	return (state==COOLING || state==COOLING_MIN_TIME);
}

/**
 * Check if current state is heating (or waiting to heat)
 */
bool TempControl::stateIsHeating(){
	return (state==HEATING || state==HEATING_MIN_TIME);
}


/**
 * \brief Get current control variables as JsonDocument
 *
 * \param doc - Reference to JsonDocument to populate
 */
void TempControl::getControlVariablesDoc(DynamicJsonDocument& doc) {
  doc["beerDiff"] = tempDiffToDouble(cv.beerDiff, Config::TempFormat::tempDiffDecimals);
  doc["diffIntegral"] = tempDiffToDouble(cv.diffIntegral, Config::TempFormat::tempDiffDecimals);
  doc["beerSlope"] = tempDiffToDouble(cv.beerSlope, Config::TempFormat::tempDiffDecimals);

  doc["p"] = fixedPointToDouble(cv.p, Config::TempFormat::fixedPointDecimals);
  doc["i"] = fixedPointToDouble(cv.i, Config::TempFormat::fixedPointDecimals);
  doc["d"] = fixedPointToDouble(cv.d, Config::TempFormat::fixedPointDecimals);

  doc["estPeak"] = tempToDouble(cv.estimatedPeak, Config::TempFormat::tempDecimals);
  doc["negPeakEst"] = tempToDouble(cv.negPeakEstimate, Config::TempFormat::tempDecimals);
  doc["posPeakEst"] = tempToDouble(cv.posPeakEstimate, Config::TempFormat::tempDecimals);
  doc["negPeak"] = tempToDouble(cv.negPeak, Config::TempFormat::tempDecimals);
  doc["posPeak"] = tempToDouble(cv.posPeak, Config::TempFormat::tempDecimals);
}

/**
 * \brief Get current control constants as JsonDocument
 *
 * \param doc - Reference to JsonDocument to populate
 */
void TempControl::getControlConstantsDoc(DynamicJsonDocument& doc) {
  doc["tempFormat"] = String(cc.tempFormat);

  doc["tempSetMin"] = tempToDouble(cc.tempSettingMin, Config::TempFormat::tempDecimals);
  doc["tempSetMax"] = tempToDouble(cc.tempSettingMax, Config::TempFormat::tempDecimals);
  doc["pidMax"] = tempDiffToDouble(cc.pidMax, Config::TempFormat::tempDiffDecimals);
  doc["Kp"] = fixedPointToDouble(cc.Kp, Config::TempFormat::fixedPointDecimals);
  doc["Ki"] = fixedPointToDouble(cc.Ki, Config::TempFormat::fixedPointDecimals);
  doc["Kd"] = fixedPointToDouble(cc.Kd, Config::TempFormat::fixedPointDecimals);

  doc["iMaxErr"] = tempDiffToDouble(cc.iMaxError, Config::TempFormat::tempDiffDecimals);
  doc["idleRangeH"] = tempDiffToDouble(cc.idleRangeHigh, Config::TempFormat::tempDiffDecimals);
  doc["idleRangeL"] = tempDiffToDouble(cc.idleRangeLow, Config::TempFormat::tempDiffDecimals);
  doc["heatTargetH"] = tempDiffToDouble(cc.heatingTargetUpper, Config::TempFormat::tempDiffDecimals);
  doc["heatTargetL"] = tempDiffToDouble(cc.heatingTargetLower, Config::TempFormat::tempDiffDecimals);
  doc["coolTargetH"] = tempDiffToDouble(cc.coolingTargetUpper, Config::TempFormat::tempDiffDecimals);
  doc["coolTargetL"] = tempDiffToDouble(cc.coolingTargetLower, Config::TempFormat::tempDiffDecimals);
  doc["maxHeatTimeForEst"] = tempControl.cc.maxHeatTimeForEstimate;
  doc["maxCoolTimeForEst"] = tempControl.cc.maxCoolTimeForEstimate;
  doc["fridgeFastFilt"] = tempControl.cc.fridgeFastFilter;
  doc["fridgeSlowFilt"] = tempControl.cc.fridgeSlowFilter;
  doc["fridgeSlopeFilt"] = tempControl.cc.fridgeSlopeFilter;
  doc["beerFastFilt"] = tempControl.cc.beerFastFilter;
  doc["beerSlowFilt"] = tempControl.cc.beerSlowFilter;
  doc["beerSlopeFilt"] = tempControl.cc.beerSlopeFilter;
  doc["lah"] = tempControl.cc.lightAsHeater;
  doc["hs"] = tempControl.cc.rotaryHalfSteps;
}


/**
 * \brief Get current control settings as a JsonDocument
 *
 * \param doc - Reference to JsonDocument to populate
 */
void TempControl::getControlSettingsDoc(DynamicJsonDocument& doc) {
  doc["mode"] = String(cs.mode);
  doc["beerSet"] = tempToDouble(cs.beerSetting, Config::TempFormat::tempDecimals);
  doc["fridgeSet"] = tempToDouble(cs.fridgeSetting, Config::TempFormat::tempDecimals);
  doc["heatEst"] = fixedPointToDouble(cs.heatEstimator, Config::TempFormat::fixedPointDecimals);
  doc["coolEst"] = fixedPointToDouble(cs.coolEstimator, Config::TempFormat::fixedPointDecimals);
}



MinTimes::MinTimes() {
	settings_choice = MIN_TIMES_DEFAULT;
	setDefaults();
}

void MinTimes::setDefaults() {
	if(settings_choice == MIN_TIMES_DEFAULT) {
		// Normal Delay
		MIN_COOL_OFF_TIME = 300;
		MIN_HEAT_OFF_TIME = 300;
		MIN_COOL_ON_TIME = 180;
		MIN_HEAT_ON_TIME = 180;

		MIN_COOL_OFF_TIME_FRIDGE_CONSTANT= 600;
		MIN_SWITCH_TIME = 600;
		COOL_PEAK_DETECT_TIME = 1800;
		HEAT_PEAK_DETECT_TIME = 900;
	} else if(settings_choice == MIN_TIMES_LOW_DELAY) {
		// Low Delay Mode
		MIN_COOL_OFF_TIME = 60;
		MIN_HEAT_OFF_TIME = 300;
		MIN_COOL_ON_TIME = 20;
		MIN_HEAT_ON_TIME = 180;

		MIN_COOL_OFF_TIME_FRIDGE_CONSTANT= 60;
		MIN_SWITCH_TIME = 600;
		COOL_PEAK_DETECT_TIME = 1800;
		HEAT_PEAK_DETECT_TIME = 900;
	} else {
		// Custom Delay -- Effectively a noop, as the defaults are set  when the json gets loaded
	}
}

uint16_t TempControl::getMinCoolOnTime() {
	return minTimes.MIN_COOL_ON_TIME;
}

uint16_t TempControl::getMinHeatOnTime() {
	return minTimes.MIN_HEAT_ON_TIME;
}


/**
 * \brief Store min times to the filesystem
 */
void MinTimes::storeToSpiffs() {
    DynamicJsonDocument doc(512);

    toJson(doc);

    writeJsonToFile(MinTimes::filename, doc);  // Write the json to the file
}

void MinTimes::loadFromSpiffs() {
    // We start by setting the defaults, as we use them as the alternative to loaded values if the keys don't exist
    setDefaults();

    DynamicJsonDocument json_doc(2048);
    json_doc = readJsonFromFile(MinTimes::filename);

	// Load the settings "default" choice from the JSON doc
	if(json_doc.containsKey(MinTimesKeys::SETTINGS_CHOICE)) settings_choice = json_doc[MinTimesKeys::SETTINGS_CHOICE];

    // Load the constants from the JSON Doc
    if(json_doc.containsKey(MinTimesKeys::MIN_COOL_OFF_TIME)) MIN_COOL_OFF_TIME = json_doc[MinTimesKeys::MIN_COOL_OFF_TIME];
    if(json_doc.containsKey(MinTimesKeys::MIN_HEAT_OFF_TIME)) MIN_HEAT_OFF_TIME = json_doc[MinTimesKeys::MIN_HEAT_OFF_TIME];
	if(json_doc.containsKey(MinTimesKeys::MIN_COOL_ON_TIME)) MIN_COOL_ON_TIME = json_doc[MinTimesKeys::MIN_COOL_ON_TIME];
	if(json_doc.containsKey(MinTimesKeys::MIN_HEAT_ON_TIME)) MIN_HEAT_ON_TIME = json_doc[MinTimesKeys::MIN_HEAT_ON_TIME];
	
	if(json_doc.containsKey(MinTimesKeys::MIN_COOL_OFF_TIME_FRIDGE_CONSTANT)) MIN_COOL_OFF_TIME_FRIDGE_CONSTANT = json_doc[MinTimesKeys::MIN_COOL_OFF_TIME_FRIDGE_CONSTANT];
	if(json_doc.containsKey(MinTimesKeys::MIN_SWITCH_TIME)) MIN_SWITCH_TIME = json_doc[MinTimesKeys::MIN_SWITCH_TIME];
	if(json_doc.containsKey(MinTimesKeys::COOL_PEAK_DETECT_TIME)) COOL_PEAK_DETECT_TIME = json_doc[MinTimesKeys::COOL_PEAK_DETECT_TIME];
	if(json_doc.containsKey(MinTimesKeys::HEAT_PEAK_DETECT_TIME)) HEAT_PEAK_DETECT_TIME = json_doc[MinTimesKeys::HEAT_PEAK_DETECT_TIME];
}



/**
 * \brief Serialize min times to JSON
 */
void MinTimes::toJson(DynamicJsonDocument &doc) {
    // Load the constants into the JSON Doc
	doc[MinTimesKeys::SETTINGS_CHOICE] = settings_choice;

    doc[MinTimesKeys::MIN_COOL_OFF_TIME] = MIN_COOL_OFF_TIME;
    doc[MinTimesKeys::MIN_HEAT_OFF_TIME] = MIN_HEAT_OFF_TIME;
	doc[MinTimesKeys::MIN_COOL_ON_TIME] = MIN_COOL_ON_TIME;
	doc[MinTimesKeys::MIN_HEAT_ON_TIME] = MIN_HEAT_ON_TIME;

	doc[MinTimesKeys::MIN_COOL_OFF_TIME_FRIDGE_CONSTANT] = MIN_COOL_OFF_TIME_FRIDGE_CONSTANT;
	doc[MinTimesKeys::MIN_SWITCH_TIME] = MIN_SWITCH_TIME;
	doc[MinTimesKeys::COOL_PEAK_DETECT_TIME] = COOL_PEAK_DETECT_TIME;
	doc[MinTimesKeys::HEAT_PEAK_DETECT_TIME] = HEAT_PEAK_DETECT_TIME;
}