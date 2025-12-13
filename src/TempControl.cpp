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
#include <math.h>

#include "TemperatureFormats.h"
#include "TempControl.h"
#include "PiLink.h"
#include "TempSensor.h"
#include "Ticks.h"
#include "TempSensorMock.h"
#include "EepromManager.h"
#include "TempSensorDisconnected.h"
#include "RotaryEncoder.h"
#include "GlycolLog.h"

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
temperature ambientTemp = TEMP_SENSOR_DISCONNECTED;  // Updated in the updateTemperatures() function to prevent reading from the sensor in an async web response


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

// Glycol mode: Predictive bang-bang control parameters
GlycolLearnedParams TempControl::glycolLearned;
GlycolConfig TempControl::glycolConfig;
GlycolRuntimeState TempControl::glycolRuntime;
    
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

    // Initialize glycol mode parameters
    loadGlycolParams();

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
 * Get the current cached room temperature.
 *
 * @return Current cached room temperature
 */
temperature TempControl::getRoomTemp() {
    return ambientTemp;
}


/**
 * Update all installed temp sensors.
 *
 * This updates beer, fridge & room sensors.
 */
void TempControl::updateTemperatures(){
    
    updateSensor(beerSensor);
    updateSensor(fridgeSensor);

    ambientTemp = ambientSensor->read();  // Update ambient sensor here rather to prevent being updated as part of an async web response
    
    // If no sensor is connected, this does nothing.
    // This prevents a delay in serial response because the value is not up to date.
    if(ambientTemp == TEMP_SENSOR_DISCONNECTED){
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

        // Allow PID to continue using cached filter values for up to 60 seconds during temporary disconnections.
        // The filters retain their last valid values, providing resilience against brief sensor dropouts.
        // After 60 failed reads (~60 seconds), the cached data is too stale to be reliable.
        if(beerSensor->getFailedReadCount() > 60) {
            return;
        }

        // fridgeSensor is required for compressor-based cooling - In glycol mode, only the beer sensor is required
        if(!extendedSettings.glycol && fridgeSensor->getFailedReadCount() > 60) {
            return;
        }
        
        // In compressor cooling, fridge setting is calculated with PID algorithm. Beer temperature error is input to PID
        // In glycol chilling, still calculate beer temperature error and slope - used by both modes
        cv.beerDiff =  cs.beerSetting - beerSensor->readSlowFiltered();
        cv.beerSlope = beerSensor->readSlope();

        if(extendedSettings.glycol) {
            // ===== GLYCOL MODE: Predictive Bang-Bang Control =====
            // PID is not used in glycol mode. Control is handled by updateGlycolState().
            // We still calculate beerDiff and beerSlope above for display/logging purposes.

            // Set fridgeSetting to INVALID_TEMP since it's not used in glycol mode
            cs.fridgeSetting = INVALID_TEMP;

            // Clear PID outputs (not used, but set for display consistency)
            cv.p = 0;
            cv.i = 0;
            cv.d = 0;
            cv.diffIntegral = 0;

        } else {
            // ===== COMPRESSOR MODE: Cascaded PID control =====
            // PID output is a fridge temperature setpoint

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

            // constrain to tempSettingMin or beerSetting - pidMax, whichever is lower.
            temperature lowerBound = (cs.beerSetting <= cc.tempSettingMin + cc.pidMax) ? cc.tempSettingMin : cs.beerSetting - cc.pidMax;
            // constrain to tempSettingMax or beerSetting + pidMax, whichever is higher.
            temperature upperBound = (cs.beerSetting >= cc.tempSettingMax - cc.pidMax) ? cc.tempSettingMax : cs.beerSetting + cc.pidMax;

            cs.fridgeSetting = constrain(constrainTemp16(newFridgeSetting), lowerBound, upperBound);
        }
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
    } else {
        // Check for invalid settings or disconnected sensors
        // In glycol mode, fridge sensor is optional; in compressor mode it's required
        bool fridgeRequired = !extendedSettings.glycol;
        bool fridgeInvalid = (fridgeRequired && (!fridgeSensor->isConnected() || cs.fridgeSetting == INVALID_TEMP));
        bool beerInvalid = (!beerSensor->isConnected() && tempControl.modeIsBeer());

        if(fridgeInvalid || beerInvalid) {
            // Stay idle when a required sensor is disconnected or settings are invalid
            state = IDLE;
            stayIdle = true;
        }
    }

    // ===== GLYCOL MODE STATE MACHINE =====
    // Uses predictive bang-bang control (see GLYCOL_COOLING_ALGORITHM.md)
    if(extendedSettings.glycol && tempControl.modeIsBeer() && !stayIdle) {
        updateGlycolState();
        // Glycol mode uses its own state machine - skip compressor mode logic
        return;
    }

    // ===== COMPRESSOR MODE STATE MACHINE =====
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
            } else if(fridgeFast < (cs.fridgeSetting+cc.idleRangeLow)) {  // fridge temperature is too low
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
            } else {
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
    // Peak detection auto-tuning is designed for compressor mode
    // In glycol mode, use overshoot prediction instead
    if(extendedSettings.glycol) {
        return;
    }

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
    cc.storeToFilesystem();
}

/**
 * Load control constants from EEPROM
 */
void TempControl::loadConstants(){
  // Now that control constants are an object, use that for loading/saving
  cc.loadFromFilesystem();
  initFilters();
}


/**
 * Write new settings to EEPROM to be able to reload them after a reset
 * The update functions only write to EEPROM if the value has changed
 */
void TempControl::storeSettings(){
    cs.storeToFilesystem();
    storedBeerSetting = cs.beerSetting;
}

/**
 * Read settings from EEPROM
 */
void TempControl::loadSettings(){
  cs.loadFromFilesystem();
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
    // In glycol mode, redirect fridge constant to beer constant
    // (Ideally, this won't ever get triggered, but handling it here just in case the web interface is old or out of sync)
    if(extendedSettings.glycol && newMode == Modes::fridgeConstant) {
        logDebug("Glycol mode: redirecting fridge constant to beer constant");
        newMode = Modes::beerConstant;
    }

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
void TempControl::getControlVariablesDoc(JsonDocument& doc) {
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
void TempControl::getControlConstantsDoc(JsonDocument& doc) {
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

  // Glycol mode: Separate heating PID constants (always used in glycol mode)
  doc["KpHeat"] = fixedPointToDouble(tempControl.cc.Kp_heat, Config::TempFormat::fixedPointDecimals);
  doc["KiHeat"] = fixedPointToDouble(tempControl.cc.Ki_heat, Config::TempFormat::fixedPointDecimals);
  doc["KdHeat"] = fixedPointToDouble(tempControl.cc.Kd_heat, Config::TempFormat::fixedPointDecimals);
  doc["pidMaxHeat"] = tempDiffToDouble(tempControl.cc.pidMax_heat, Config::TempFormat::tempDiffDecimals);
}


/**
 * \brief Get current control settings as a JsonDocument
 *
 * \param doc - Reference to JsonDocument to populate
 */
void TempControl::getControlSettingsDoc(JsonDocument& doc) {
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
    // Glycol mode has different timing requirements than compressor mode
    // Glycol systems can respond faster and don't need compressor protection delays
    if(extendedSettings.glycol && settings_choice != MIN_TIMES_CUSTOM) {
        // Glycol Mode - Time-proportional control with 1000s window
        MIN_COOL_OFF_TIME = 30;  // Legacy, not used in time-proportional mode
        MIN_HEAT_OFF_TIME = 30;
        MIN_COOL_ON_TIME = 30;
        MIN_HEAT_ON_TIME = 30;

        MIN_COOL_OFF_TIME_FRIDGE_CONSTANT = 30;
        MIN_SWITCH_TIME = 60;
        COOL_PEAK_DETECT_TIME = 300;
        HEAT_PEAK_DETECT_TIME = 300;

        // Time-proportional control settings
        GLYCOL_WINDOW_PERIOD = 1000;  // 1000s (~16.67 min) window for duty cycle control
        GLYCOL_MIN_ON_TIME = 10;      // 10s minimum on-time (1% minimum duty cycle)
    } else if(settings_choice == MIN_TIMES_DEFAULT) {
        // Compressor Mode - Normal Delay
        MIN_COOL_OFF_TIME = 300;
        MIN_HEAT_OFF_TIME = 300;
        MIN_COOL_ON_TIME = 180;
        MIN_HEAT_ON_TIME = 180;

        MIN_COOL_OFF_TIME_FRIDGE_CONSTANT= 600;
        MIN_SWITCH_TIME = 600;
        COOL_PEAK_DETECT_TIME = 1800;
        HEAT_PEAK_DETECT_TIME = 900;

        // Glycol settings (not used in compressor mode, but set for consistency)
        GLYCOL_WINDOW_PERIOD = 1000;
        GLYCOL_MIN_ON_TIME = 10;
    } else if(settings_choice == MIN_TIMES_LOW_DELAY) {
        // Compressor Mode - Low Delay
        MIN_COOL_OFF_TIME = 60;
        MIN_HEAT_OFF_TIME = 300;
        MIN_COOL_ON_TIME = 20;
        MIN_HEAT_ON_TIME = 180;

        MIN_COOL_OFF_TIME_FRIDGE_CONSTANT= 60;
        MIN_SWITCH_TIME = 600;
        COOL_PEAK_DETECT_TIME = 1800;
        HEAT_PEAK_DETECT_TIME = 900;

        // Glycol settings (not used in compressor mode, but set for consistency)
        GLYCOL_WINDOW_PERIOD = 1000;
        GLYCOL_MIN_ON_TIME = 10;
    } else {
        // Custom Delay -- Effectively a noop, as the defaults are set when the json gets loaded
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
void MinTimes::storeToFilesystem() {
    JsonDocument doc;

    toJson(doc);

    writeJsonToFile(MinTimes::filename, doc);  // Write the json to the file
}

void MinTimes::loadFromFilesystem() {
    // We start by setting the defaults, as we use them as the alternative to loaded values if the keys don't exist
    setDefaults();

    JsonDocument json_doc;
    json_doc = readJsonFromFile(MinTimes::filename);

    // Load the settings "default" choice from the JSON doc
    if(json_doc[MinTimesKeys::SETTINGS_CHOICE].is<MinTimesSettingsChoice>()) settings_choice = json_doc[MinTimesKeys::SETTINGS_CHOICE];

    // Load the constants from the JSON Doc
    if(json_doc[MinTimesKeys::MIN_COOL_OFF_TIME].is<uint16_t>()) MIN_COOL_OFF_TIME = json_doc[MinTimesKeys::MIN_COOL_OFF_TIME];
    if(json_doc[MinTimesKeys::MIN_HEAT_OFF_TIME].is<uint16_t>()) MIN_HEAT_OFF_TIME = json_doc[MinTimesKeys::MIN_HEAT_OFF_TIME];
    if(json_doc[MinTimesKeys::MIN_COOL_ON_TIME].is<uint16_t>()) MIN_COOL_ON_TIME = json_doc[MinTimesKeys::MIN_COOL_ON_TIME];
    if(json_doc[MinTimesKeys::MIN_HEAT_ON_TIME].is<uint16_t>()) MIN_HEAT_ON_TIME = json_doc[MinTimesKeys::MIN_HEAT_ON_TIME];
    
    if(json_doc[MinTimesKeys::MIN_COOL_OFF_TIME_FRIDGE_CONSTANT].is<uint16_t>()) MIN_COOL_OFF_TIME_FRIDGE_CONSTANT = json_doc[MinTimesKeys::MIN_COOL_OFF_TIME_FRIDGE_CONSTANT];
    if(json_doc[MinTimesKeys::MIN_SWITCH_TIME].is<uint16_t>()) MIN_SWITCH_TIME = json_doc[MinTimesKeys::MIN_SWITCH_TIME];
    if(json_doc[MinTimesKeys::COOL_PEAK_DETECT_TIME].is<uint16_t>()) COOL_PEAK_DETECT_TIME = json_doc[MinTimesKeys::COOL_PEAK_DETECT_TIME];
    if(json_doc[MinTimesKeys::HEAT_PEAK_DETECT_TIME].is<uint16_t>()) HEAT_PEAK_DETECT_TIME = json_doc[MinTimesKeys::HEAT_PEAK_DETECT_TIME];

    // Glycol mode time-proportional control settings
    if(json_doc[MinTimesKeys::GLYCOL_WINDOW_PERIOD].is<uint16_t>()) GLYCOL_WINDOW_PERIOD = json_doc[MinTimesKeys::GLYCOL_WINDOW_PERIOD];
    if(json_doc[MinTimesKeys::GLYCOL_MIN_ON_TIME].is<uint16_t>()) GLYCOL_MIN_ON_TIME = json_doc[MinTimesKeys::GLYCOL_MIN_ON_TIME];
}



/**
 * \brief Serialize min times to JSON
 */
void MinTimes::toJson(JsonDocument &doc) {
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

    // Glycol mode time-proportional control settings
    doc[MinTimesKeys::GLYCOL_WINDOW_PERIOD] = GLYCOL_WINDOW_PERIOD;
    doc[MinTimesKeys::GLYCOL_MIN_ON_TIME] = GLYCOL_MIN_ON_TIME;
}


// ============================================================================
// GLYCOL MODE: Predictive Bang-Bang Control Implementation
// See GLYCOL_COOLING_ALGORITHM.md for design documentation
// ============================================================================



// ----- GlycolRuntimeState -----

void GlycolRuntimeState::reset() {
    state = GLYCOL_IDLE;
    t_pump_on = 0;
    t_pump_off = 0;
    emergency_entry_time = 0;
    temp_at_pump_on = 0;
    temp_at_pump_off = 0;
    min_temp_reached = 0;
    cooling_rate_at_pump_off = 0;
    current_cooling_rate = 0;
    cooling_confirmed = false;
    negative_rate_count = 0;
    setpoint_changed_this_cycle = false;
    cooling_duration_s = 0;
    force_minimum_cooling = false;
    rate_buffer_head = 0;
    rate_buffer_count = 0;
}

// ----- TempControl Glycol Methods -----

void TempControl::loadGlycolParams() {
    glycolLearned.loadFromFilesystem();
    glycolConfig.loadFromFilesystem();
    glycolRuntime.reset();
}

void TempControl::storeGlycolParams() {
    glycolLearned.storeToFilesystem();
}

/**
 * Add a temperature sample to the rate calculation buffer
 */
void TempControl::glycolAddRateSample(float temp) {
    uint32_t now = millis();

    // Add to circular buffer
    glycolRuntime.rate_buffer[glycolRuntime.rate_buffer_head].timestamp_ms = now;
    glycolRuntime.rate_buffer[glycolRuntime.rate_buffer_head].temp = temp;

    glycolRuntime.rate_buffer_head = (glycolRuntime.rate_buffer_head + 1) % RATE_BUFFER_SIZE;
    if (glycolRuntime.rate_buffer_count < RATE_BUFFER_SIZE) {
        glycolRuntime.rate_buffer_count++;
    }
}

/**
 * Calculate temperature rate of change using linear regression
 * Returns rate in degrees per minute
 */
float TempControl::glycolCalculateRate() {
    if (glycolRuntime.rate_buffer_count < 5) {
        return 0.0f;  // Not enough data
    }

    // Find oldest and newest samples to check for stale data
    uint8_t oldest_idx = (glycolRuntime.rate_buffer_head + RATE_BUFFER_SIZE - glycolRuntime.rate_buffer_count) % RATE_BUFFER_SIZE;
    uint8_t newest_idx = (glycolRuntime.rate_buffer_head + RATE_BUFFER_SIZE - 1) % RATE_BUFFER_SIZE;

    uint32_t time_span = glycolRuntime.rate_buffer[newest_idx].timestamp_ms -
                         glycolRuntime.rate_buffer[oldest_idx].timestamp_ms;

    if (time_span > 180000) {  // > 3 minutes of data is stale
        return 0.0f;
    }

    // Linear regression: temp = rate * time + intercept
    float sum_t = 0, sum_temp = 0, sum_t2 = 0, sum_t_temp = 0;
    float t0 = glycolRuntime.rate_buffer[oldest_idx].timestamp_ms;
    int n = glycolRuntime.rate_buffer_count;

    for (int i = 0; i < n; i++) {
        uint8_t idx = (oldest_idx + i) % RATE_BUFFER_SIZE;
        float t = (glycolRuntime.rate_buffer[idx].timestamp_ms - t0) / 60000.0f;  // minutes
        float temp = glycolRuntime.rate_buffer[idx].temp;
        sum_t += t;
        sum_temp += temp;
        sum_t2 += t * t;
        sum_t_temp += t * temp;
    }

    float denominator = n * sum_t2 - sum_t * sum_t;
    if (fabsf(denominator) < 0.0001f) {
        return 0.0f;  // Division by zero guard
    }

    float rate = (n * sum_t_temp - sum_t * sum_temp) / denominator;
    return rate;  // degrees per minute
}

/**
 * Estimate coast amount using hybrid model
 * Uses k * |rate| when rate is substantial, C_off otherwise
 */
float TempControl::glycolEstimateCoast() {
    float rate = fabsf(glycolRuntime.current_cooling_rate);

    if (rate > glycolConfig.min_rate_for_k_model) {
        // Rate is substantial - use rate-dependent model
        return glycolLearned.k * rate;
    } else {
        // Rate is tiny - use average coast
        return glycolLearned.C_off;
    }
}

/**
 * Check if we should start cooling
 * Based on anticipated peak temperature
 */
bool TempControl::glycolShouldStartCooling() {
    if (cs.beerSetting == INVALID_TEMP) return false;

    float current_temp = tempToDouble(beerSensor->readFastFiltered(), 2);
    float setpoint = tempToDouble(cs.beerSetting, 2);

    // Calculate lookahead time: dead time L + buffer
    float lookahead_s = glycolLearned.L + 30.0f;  // L + 30s buffer
    float lookahead_min = lookahead_s / 60.0f;

    // Calculate anticipated peak
    float anticipated_peak = current_temp + (glycolLearned.drift_rate * lookahead_min);

    // Start cooling if anticipated peak exceeds setpoint + trigger margin
    return anticipated_peak >= (setpoint + glycolConfig.trigger_margin);
}

/**
 * Check if we should stop cooling
 * Based on predicted final temperature after coast
 */
bool TempControl::glycolShouldStopCooling() {
    if (cs.beerSetting == INVALID_TEMP) return false;

    float current_temp = tempToDouble(beerSensor->readFastFiltered(), 2);
    float setpoint = tempToDouble(cs.beerSetting, 2);

    // Estimate where temperature will end up if we stop now
    float estimated_coast = glycolEstimateCoast();
    float predicted_final = current_temp - estimated_coast;

    return predicted_final <= setpoint;
}

/**
 * Check for emergency condition (can't cool fast enough)
 * Uses horizon-based prediction instead of instantaneous rate
 */
bool TempControl::glycolIsEmergency() {
    if (cs.beerSetting == INVALID_TEMP) return false;

    float current_temp = tempToDouble(beerSensor->readFastFiltered(), 2);
    float setpoint = tempToDouble(cs.beerSetting, 2);

    // We're already at or below setpoint - not an emergency
    if (current_temp <= setpoint) return false;

    // Check if we've been cooling long enough to see an effect
    if (glycolRuntime.cooling_duration_s < glycolConfig.emergency_detection_time_s) {
        return false;
    }

    // Predict temperature at horizon if we continue cooling
    float predicted_at_horizon = current_temp + (glycolRuntime.current_cooling_rate * glycolConfig.emergency_horizon_min);

    // If we can't reach setpoint even in horizon_min of continuous cooling, it's emergency
    return predicted_at_horizon > (setpoint + 0.1f);  // 0.1 margin
}

/**
 * Check if we can exit emergency mode
 */
bool TempControl::glycolCanExitEmergency() {
    if (cs.beerSetting == INVALID_TEMP) return false;

    float current_temp = tempToDouble(beerSensor->readFastFiltered(), 2);
    float setpoint = tempToDouble(cs.beerSetting, 2);

    // Check minimum dwell time
    uint32_t emergency_duration = (millis() - glycolRuntime.emergency_entry_time) / 1000;
    if (emergency_duration < glycolConfig.min_emergency_dwell_time_s) {
        return false;
    }

    // Exit if we've reached setpoint
    if (current_temp <= setpoint) {
        return true;
    }

    // Exit if cooling is now effective (meaningful negative rate)
    // and we're predicted to reach setpoint within a reasonable time
    if (glycolRuntime.current_cooling_rate < -glycolConfig.min_rate_for_k_model) {
        // Predict time to reach setpoint at current rate
        float temp_diff = current_temp - setpoint;
        float rate = fabsf(glycolRuntime.current_cooling_rate);
        if (rate > 0.001f) {
            float time_to_setpoint = temp_diff / rate;  // minutes
            if (time_to_setpoint < glycolConfig.emergency_horizon_min / 2.0f) {
                return true;  // We'll reach setpoint in reasonable time
            }
        }
    }

    return false;
}

/**
 * Transition to GLYCOL_IDLE state
 */
void TempControl::glycolTransitionToIdle() {
    GlycolState prev_state = glycolRuntime.state;
    glycolRuntime.state = GLYCOL_IDLE;
    glycolRuntime.setpoint_changed_this_cycle = false;
    state = IDLE;
    lastIdleTime = ticks.seconds();

    // Log transition
#ifdef ENABLE_GLYCOL_LOGGING
    float current_temp = tempToDouble(beerSensor->readFastFiltered(), 2);
    float setpoint = tempToDouble(cs.beerSetting, 2);
    glycolLog.logTransition(
        prev_state, GLYCOL_IDLE,
        current_temp, setpoint,
        glycolRuntime.current_cooling_rate,
        glycolRuntime.cooling_duration_s,
        glycolEstimateCoast(),
        glycolLearned.k, glycolLearned.C_off, glycolLearned.L,
        glycolRuntime.force_minimum_cooling,
        "Transition to idle"
    );
#endif
}

/**
 * Transition to GLYCOL_COOLING state
 */
void TempControl::glycolTransitionToCooling() {
    GlycolState prev_state = glycolRuntime.state;
    float setpoint = tempToDouble(cs.beerSetting, 2);

    glycolRuntime.state = GLYCOL_COOLING;
    glycolRuntime.t_pump_on = millis();
    glycolRuntime.temp_at_pump_on = tempToDouble(beerSensor->readFastFiltered(), 2);
    glycolRuntime.cooling_confirmed = false;
    glycolRuntime.negative_rate_count = 0;
    glycolRuntime.setpoint_changed_this_cycle = false;
    glycolRuntime.cooling_duration_s = 0;

    // Clear rate buffer for fresh measurements
    glycolRuntime.rate_buffer_count = 0;
    glycolRuntime.rate_buffer_head = 0;

    state = COOLING;
    lastCoolTime = ticks.seconds();

    // Log transition
#ifdef ENABLE_GLYCOL_LOGGING
    const char* reason = glycolRuntime.force_minimum_cooling
        ? "Starting cooling (forced minimum)"
        : "Starting cooling (prediction-based)";
    glycolLog.logTransition(
        prev_state, GLYCOL_COOLING,
        glycolRuntime.temp_at_pump_on, setpoint,
        glycolRuntime.current_cooling_rate,
        0,  // cooling_duration_s is 0 at start
        glycolEstimateCoast(),
        glycolLearned.k, glycolLearned.C_off, glycolLearned.L,
        glycolRuntime.force_minimum_cooling,
        reason
    );
#endif
}

/**
 * Transition to GLYCOL_COASTING state
 */
void TempControl::glycolTransitionToCoasting() {
    GlycolState prev_state = glycolRuntime.state;
    float setpoint = tempToDouble(cs.beerSetting, 2);

    glycolRuntime.state = GLYCOL_COASTING;
    glycolRuntime.t_pump_off = millis();
    glycolRuntime.temp_at_pump_off = tempToDouble(beerSensor->readFastFiltered(), 2);
    glycolRuntime.min_temp_reached = glycolRuntime.temp_at_pump_off;
    glycolRuntime.cooling_rate_at_pump_off = glycolRuntime.current_cooling_rate;
    glycolRuntime.cooling_duration_s = (glycolRuntime.t_pump_off - glycolRuntime.t_pump_on) / 1000;

    // Hot glycol compensation: during long runs, hot beer warms the glycol reservoir.
    // After pump stops, the chiller cools the reservoir back to its setpoint.
    // Next cycle will have cold glycol - force minimum time and re-learn.
    const char* reason;
    if (glycolRuntime.cooling_duration_s > glycolConfig.hot_glycol_threshold_s) {
        glycolRuntime.force_minimum_cooling = true;
        reason = "Stopping cooling (long run - forcing min next)";
    } else {
        reason = "Stopping cooling (coast prediction)";
    }

    state = IDLE;
    lastIdleTime = ticks.seconds();

    // Log transition
#ifdef ENABLE_GLYCOL_LOGGING
    glycolLog.logTransition(
        prev_state, GLYCOL_COASTING,
        glycolRuntime.temp_at_pump_off, setpoint,
        glycolRuntime.cooling_rate_at_pump_off,
        glycolRuntime.cooling_duration_s,
        glycolEstimateCoast(),
        glycolLearned.k, glycolLearned.C_off, glycolLearned.L,
        glycolRuntime.force_minimum_cooling,
        reason
    );
#endif
}

/**
 * Transition to GLYCOL_EMERGENCY_COOLING state
 */
void TempControl::glycolTransitionToEmergency() {
    GlycolState prev_state = glycolRuntime.state;
    float current_temp = tempToDouble(beerSensor->readFastFiltered(), 2);
    float setpoint = tempToDouble(cs.beerSetting, 2);

    glycolRuntime.state = GLYCOL_EMERGENCY_COOLING;
    glycolRuntime.emergency_entry_time = millis();

    state = COOLING;
    lastCoolTime = ticks.seconds();

    // Log transition
#ifdef ENABLE_GLYCOL_LOGGING
    glycolLog.logTransition(
        prev_state, GLYCOL_EMERGENCY_COOLING,
        current_temp, setpoint,
        glycolRuntime.current_cooling_rate,
        glycolRuntime.cooling_duration_s,
        glycolEstimateCoast(),
        glycolLearned.k, glycolLearned.C_off, glycolLearned.L,
        glycolRuntime.force_minimum_cooling,
        "EMERGENCY - cannot keep up with cooling demand"
    );
#endif
}

/**
 * Update learned parameters after a cooling cycle
 */
void TempControl::glycolUpdateLearning() {
    // Clear force_minimum_cooling flag - we've completed a cycle and can resume normal prediction
    if (glycolRuntime.force_minimum_cooling) {
        glycolRuntime.force_minimum_cooling = false;
        logDebug("Glycol: Cleared force_minimum_cooling after cycle");
    }

    // Check cycle validity for training
    bool cycle_valid =
        glycolRuntime.cooling_duration_s >= glycolConfig.min_training_duration_s &&
        (glycolRuntime.temp_at_pump_on - glycolRuntime.temp_at_pump_off) >= glycolConfig.min_training_drop &&
        !glycolRuntime.setpoint_changed_this_cycle &&
        fabsf(glycolRuntime.cooling_rate_at_pump_off) > glycolConfig.min_training_rate;

    if (!cycle_valid) {
        logDebug("Glycol: Cycle not valid for training");
        return;
    }

    float actual_coast = glycolRuntime.temp_at_pump_off - glycolRuntime.min_temp_reached;
    float setpoint = tempToDouble(cs.beerSetting, 2);

    // Always update C_off for valid cycles
    glycolLearned.C_off = 0.85f * glycolLearned.C_off + 0.15f * actual_coast;
    glycolLearned.C_off = constrain(glycolLearned.C_off, 0.05f, 2.0f);

    // Update k if rate was meaningful
    if (fabsf(glycolRuntime.cooling_rate_at_pump_off) > glycolConfig.min_training_rate) {
        float observed_k = actual_coast / fabsf(glycolRuntime.cooling_rate_at_pump_off);

        // Clamp observed_k before using
        observed_k = constrain(observed_k, 0.5f, 20.0f);

        // Sanity check against C_off
        float implied_coast_at_typical_rate = observed_k * 0.05f;
        float alpha;

        if (implied_coast_at_typical_rate > glycolLearned.C_off * 3.0f ||
            implied_coast_at_typical_rate < glycolLearned.C_off * 0.3f) {
            // This k seems unreasonable - reduce learning rate
            alpha = 0.05f;
        } else {
            // Normal adaptive learning rate based on prediction error
            float prediction_error = setpoint - glycolRuntime.min_temp_reached;

            if (fabsf(prediction_error) > 0.5f) {
                alpha = 0.5f;   // Big miss - adapt fast
            } else if (fabsf(prediction_error) > 0.2f) {
                alpha = 0.3f;   // Medium miss
            } else {
                alpha = 0.15f;  // Small miss - fine tuning
            }
        }

        // Update k with exponential moving average
        glycolLearned.k = (1.0f - alpha) * glycolLearned.k + alpha * observed_k;
        glycolLearned.k = constrain(glycolLearned.k, 1.0f, 15.0f);
    }

    logDebug("Glycol: Learning update - k=%.2f, C_off=%.3f", glycolLearned.k, glycolLearned.C_off);

    // Persist learned parameters
    storeGlycolParams();
}

/**
 * Main glycol state machine update
 * Called from updateState() when in glycol mode
 */
void TempControl::updateGlycolState() {
    if (!extendedSettings.glycol || !modeIsBeer()) return;
    if (cs.beerSetting == INVALID_TEMP) {
        glycolTransitionToIdle();
        return;
    }

    // Get current temperature and add to rate buffer
    float current_temp = tempToDouble(beerSensor->readFastFiltered(), 2);
    float setpoint = tempToDouble(cs.beerSetting, 2);
    glycolAddRateSample(current_temp);

    // Update current cooling rate
    glycolRuntime.current_cooling_rate = glycolCalculateRate();

    // Safety check: temperature too low
    if (current_temp < (setpoint - glycolConfig.safety_margin_low)) {
        if (glycolRuntime.state != GLYCOL_IDLE && glycolRuntime.state != GLYCOL_COASTING) {
            logDebug("Glycol: Safety limit - beer too cold");
            glycolTransitionToIdle();
            return;
        }
    }

    switch (glycolRuntime.state) {
        case GLYCOL_IDLE: {
            // Update drift rate while idle
            if (glycolRuntime.current_cooling_rate > 0) {  // Only update if warming
                glycolLearned.drift_rate = 0.85f * glycolLearned.drift_rate +
                                           0.15f * glycolRuntime.current_cooling_rate;
                glycolLearned.drift_rate = constrain(glycolLearned.drift_rate, 0.0f, 0.5f);
            }

            // Check if we should start cooling
            uint16_t time_since_pump_off = (millis() - glycolRuntime.t_pump_off) / 1000;
            bool min_off_elapsed = (glycolRuntime.t_pump_off == 0) ||
                                   (time_since_pump_off >= glycolConfig.min_off_time_s);

            if (min_off_elapsed && glycolShouldStartCooling()) {
                glycolTransitionToCooling();
            }
            break;
        }

        case GLYCOL_COOLING: {
            // Update cooling duration
            glycolRuntime.cooling_duration_s = (millis() - glycolRuntime.t_pump_on) / 1000;

            // Dead time learning: detect when cooling starts taking effect
            if (!glycolRuntime.cooling_confirmed) {
                if (glycolRuntime.current_cooling_rate < -0.01f) {  // Negative rate threshold
                    glycolRuntime.negative_rate_count++;
                    if (glycolRuntime.negative_rate_count >= 3) {
                        // Cooling confirmed
                        float L_observed = (millis() - glycolRuntime.t_pump_on) / 1000.0f;
                        L_observed = constrain(L_observed, 5.0f, 120.0f);
                        glycolLearned.L = 0.8f * glycolLearned.L + 0.2f * L_observed;
                        glycolRuntime.cooling_confirmed = true;
                    }
                } else {
                    glycolRuntime.negative_rate_count = 0;
                }
            }

            // Safety: max continuous on time
            uint32_t max_on_s = glycolConfig.max_continuous_on_time_min * 60;
            if (glycolRuntime.cooling_duration_s > max_on_s) {
                logDebug("Glycol: Max continuous on time exceeded");
                glycolTransitionToCoasting();
                return;
            }

            // Check for emergency condition
            if (glycolIsEmergency()) {
                glycolTransitionToEmergency();
                return;
            }

            // Check if minimum on time has elapsed before making stop decisions
            if (glycolRuntime.cooling_duration_s < glycolConfig.min_on_time_s) {
                state = COOLING_MIN_TIME;
                break;
            }
            state = COOLING;

            // If force_minimum_cooling is set (after a long run warmed the reservoir),
            // stop immediately after min_on_time and let the learning adapt
            if (glycolRuntime.force_minimum_cooling) {
                logDebug("Glycol: Forced minimum cooling - stopping to re-learn");
                glycolTransitionToCoasting();
                break;
            }

            // Check if we should stop cooling
            if (glycolShouldStopCooling()) {
                glycolTransitionToCoasting();
            }
            break;
        }

        case GLYCOL_COASTING: {
            // Track minimum temperature
            if (current_temp < glycolRuntime.min_temp_reached) {
                glycolRuntime.min_temp_reached = current_temp;
            }

            // Check if temperature has stabilized or started rising
            bool stabilized = (glycolRuntime.current_cooling_rate >= -0.005f);  // Near zero or positive

            uint16_t time_since_pump_off = (millis() - glycolRuntime.t_pump_off) / 1000;
            bool min_off_elapsed = time_since_pump_off >= glycolConfig.min_off_time_s;

            if (stabilized && min_off_elapsed) {
                // Perform learning update
                glycolUpdateLearning();

                // Transition to idle
                glycolTransitionToIdle();
            }
            break;
        }

        case GLYCOL_EMERGENCY_COOLING: {
            // Update cooling duration for safety check
            uint32_t emergency_duration = (millis() - glycolRuntime.emergency_entry_time) / 1000;

            // Safety: max continuous on time
            uint32_t max_on_s = glycolConfig.max_continuous_on_time_min * 60;
            if (emergency_duration > max_on_s) {
                logDebug("Glycol: Max on time in emergency, forcing off");
                glycolTransitionToCoasting();
                return;
            }

            // Safety: temperature too low
            if (current_temp < (setpoint - glycolConfig.safety_margin_low)) {
                logDebug("Glycol: Safety limit in emergency");
                glycolTransitionToIdle();
                return;
            }

            // Check if we can exit emergency
            if (glycolCanExitEmergency()) {
                logDebug("Glycol: Exiting emergency mode");
                glycolTransitionToCooling();  // Return to normal predictive control
            }
            break;
        }
    }
}