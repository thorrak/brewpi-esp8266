/*
 * Copyright 2013 BrewPi/Elco Jacobs.
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
#include "Simulator.h"

#include "Display.h"
#include "PiLink.h"
#include "CommandProcessor.h"
#include <esp_timer.h>

#if BREWPI_SIMULATE

Simulator simulator;

static temperature funFactor = 0;	// paused
static unsigned long lastUpdate = 0;
uint8_t printTempInterval = 5;

void setRunFactor(temperature factor)
{
	funFactor = factor>>9;		// for now whole values only
	lastUpdate = (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void updateSimulationTicks()
{
#if BREWPI_EMULATE
	// in the avr simulator (we call emulator to try to distinguish), ticks take forever. 1 second takes many minutes if
	// emulating waiting for millis() to increment.
	ticks.incMillis(1000);
#else
	if (funFactor) {
		unsigned long now = (unsigned long)(esp_timer_get_time() / 1000ULL);
		int interval = 1000/funFactor;
		if (interval>0) {
			if ((now-lastUpdate)>=uint16_t(interval)) {
				lastUpdate += interval;
				ticks.incMillis(1000);
			}
		}
		else
		{
			lastUpdate = now;
			ticks.incMillis(1000);
		}
	}
#endif
}


/**
 * \brief Main execution loop when in simulation mode
 */
void simulateLoop()
{
	static unsigned long lastUpdate = 0;

	// only needed if we want the arduino to be self running. Useful for manual testing, but not so much with an
	// external driver.
	updateSimulationTicks();

	if(ticks.millis() - lastUpdate >= (1000)) { //update settings every second
		lastUpdate = ticks.millis();
		
		tempControl.updateTemperatures();
		tempControl.detectPeaks();
		tempControl.updatePID();
		tempControl.updateState();
		tempControl.updateOutputs();

		#if !BREWPI_EMULATE			// simulation on actual hardware
		static byte updateCount = 0;
		if (printTempInterval && (++updateCount%printTempInterval)==0) {
			piLink.printTemperatures(nullptr, nullptr);
			updateCount = 0;
		}
		static unsigned long lastDisplayUpdate = 0;  // update the display every second
		if (((unsigned long)(esp_timer_get_time() / 1000ULL)-lastDisplayUpdate)>=1000 && (lastDisplayUpdate+=1000))
		#endif
		{
			// update the lcd for the chamber being displayed
			display.printAll();

#ifdef BREWPI_IIC
			display.updateBacklight();
#endif
		}
		
		simulator.step();
	}
	#if !BREWPI_EMULATE
	static unsigned long lastCheckSerial = 0;
	if (((unsigned long)(esp_timer_get_time() / 1000ULL)-lastCheckSerial)>=1000 && (lastCheckSerial=(unsigned long)(esp_timer_get_time() / 1000ULL)>0))	// only listen if 1s passed since last time
	#endif
	//listen for incoming serial connections while waiting to update
	CommandProcessor::receiveCommand();
}

#include "TempSensorExternal.h"

const char SimulatorBeerTemp[] = "b";
const char SimulatorBeerConnected[] = "bc";
const char SimulatorBeerVolume[] = "bv";
const char SimulatorCoolPower[] = "c";
const char SimulatorDoorState[] = "d";
const char SimulatorEnabled[] = "e";
const char SimulatorFridgeTemp[] = "f";
const char SimulatorFridgeConnected[] = "fc";
const char SimulatorFridgeVolume[] = "fv";
const char SimulatorHeatPower[] = "h";
const char SimulatorPrintInterval[] = "i";
const char SimulatorNoise[] = "n";
const char SimulatorCoeffBeer[] = "kb";
const char SimulatorCoeffRoom[] = "ke";
const char SimulatorRoomTempMin[] = "rmi";
const char SimulatorRoomTempMax[] = "rmx";
const char SimulatorBeerDensity[] = "sg";
const char SimulatorTime[] = "t";


void setTicks(ExternalTicks& externalTicks, const char* val, int multiplier=1000) {		
	
	if (val==NULL || *val==0) {
		externalTicks.incMillis(1000);
	}
	else {
		if (*val=='=')
			externalTicks.setMillis(atol(val+1)*multiplier);
		else
			externalTicks.incMillis(atol(val+1)*multiplier);
	}
	
	logDebug("New ticks %lu", externalTicks.millis());
}


/* How often the temperature is output, in simulated seconds.
 * 0 is never.
 * 1 is once per second.
 * 5 is once every 5 seconds etc..
 */
extern uint8_t printTempInterval;


void HandleSimulatorConfig(const char* key, const char* val, void* pv)
{
	// this set the system timer, but not the simulator counter
	if (strcmp(key, "s")==0) {
		setTicks(ticks, val, 1000);
	}
	// these are all doubles - could replace this with a map of string keys to methods
	else if (strcmp(key, SimulatorRoomTempMin)==0) {
		simulator.setMinRoomTemp(atof(val));
	}
	else if (strcmp(key, SimulatorRoomTempMax)==0) {
		simulator.setMaxRoomTemp(atof(val));
	}
	else if (strcmp(key, SimulatorFridgeVolume)==0) {
		simulator.setFridgeVolume(atof(val));
	}
	else if (strcmp(key, SimulatorBeerVolume)==0) {
		simulator.setBeerVolume(atof(val));
	}
	else if (strcmp(key, SimulatorBeerDensity)==0) {
		simulator.setBeerDensity(atof(val));
	}
	else if (strcmp(key, SimulatorFridgeTemp)==0) {
		simulator.setFridgeTemp(atof(val));
	}
	else if (strcmp(key, SimulatorBeerTemp)==0) {
		simulator.setBeerTemp(atof(val));
	}
	else if (strcmp(key, SimulatorHeatPower)==0) {
		simulator.setHeatPower(atof(val));
	}
	else if (strcmp(key, SimulatorCoolPower)==0) {
		simulator.setCoolPower(atof(val));
	}
	else if (strcmp(key, SimulatorCoeffRoom)==0) {
		simulator.setRoomCoefficient(atof(val));
	}
	else if (strcmp(key, SimulatorCoeffBeer)==0) {
		simulator.setBeerCoefficient(atof(val));
	}
	else if (strcmp(key, SimulatorBeerConnected)==0) {
		simulator.setConnected(tempControl.beerSensor, strcmp(val, "0")!=0);
	}
	else if (strcmp(key, SimulatorFridgeConnected)==0) {
		simulator.setConnected(tempControl.fridgeSensor, strcmp(val, "0")!=0);
	}		
	else if (strcmp(key, SimulatorDoorState)==0) {		// 0 for closed, anything else for open
		simulator.setSwitch(tempControl.door, strcmp(val, "0")!=0);
	}
	else if (strcmp(key, "r")==0) {
		setRunFactor(stringToFixedPoint(val));
	}
	else if (!strcmp(key, SimulatorPrintInterval)) {
		printTempInterval = atol(val);
	}
	else if (!strcmp(key, SimulatorNoise)) {
		simulator.setSensorNoise(atof(val));
	}
	else if (strcmp(key, SimulatorEnabled)==0) {		// 0 for closed, anything else for open
		simulator.setSimulationEnabled(strcmp(val, "0")!=0);
	}

}


/**
 * \brief Dump current simulator settings
 */
void Simulator::printSettings()
{
  JsonDocument doc;

  doc[SimulatorRoomTempMin] = simulator.getMinRoomTemp();
	doc[SimulatorRoomTempMax] = simulator.getMaxRoomTemp();
	doc[SimulatorFridgeVolume] = simulator.getFridgeVolume();
	doc[SimulatorBeerVolume] = simulator.getBeerVolume();
	doc[SimulatorBeerDensity] = simulator.getBeerDensity();
	doc[SimulatorFridgeTemp] = simulator.getFridgeTemp();
	doc[SimulatorBeerTemp] = simulator.getBeerTemp();
	doc[SimulatorFridgeConnected] = simulator.getConnected(tempControl.fridgeSensor) ? "1" : "0";
	doc[SimulatorBeerConnected] = simulator.getConnected(tempControl.beerSensor) ? "1" : "0";
	doc[SimulatorHeatPower] = (uint16_t)simulator.getHeatPower();
	doc[SimulatorCoolPower] = (uint16_t)simulator.getCoolPower();
	doc[SimulatorCoeffRoom] = simulator.getRoomCoefficient();
 	doc[SimulatorCoeffBeer] = simulator.getBeerCoefficient();
	doc[SimulatorDoorState] = simulator.doorState() ? "1" : "0";
	doc[SimulatorDoorState] = printTempInterval;
  doc[SimulatorNoise] = simulator.getSensorNoise();

  piLink.sendJsonMessage('Y', doc);
}


/**
 * \brief Parse simulator settings
 *
 * \todo Implement this feature
 */
void Simulator::parseSettings() {

}

#endif // brewpi simulate
