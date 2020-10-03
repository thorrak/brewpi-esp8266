/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
 *
 * This file is part of BrewPi.
 * 
 * BrewPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later v7ersion.
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
#include "BrewpiStrings.h"
#include <limits.h>
#include <stdint.h>

// If we're using BREWPI_TFT, use that code instead
#ifndef BREWPI_TFT

#include "Display.h"
#include "DisplayLcd.h"
#include "Menu.h"
#include "TempControl.h"
#include "TemperatureFormats.h"
#include "Pins.h"

#ifdef ESP8266_WiFi

#if defined(ESP8266)
#include <ESP8266WiFi.h>  // For printing the IP address
#elif defined(ESP32)
#include <WiFi.h> // For printing the IP address
#else
#error "Invalid chipset!"
#endif

#endif


uint8_t LcdDisplay::stateOnDisplay;
uint8_t LcdDisplay::flags;
#if defined(BREWPI_IIC)
LcdDriver LcdDisplay::lcd(0x27, Config::Lcd::columns, Config::Lcd::lines);  // NOTE - The address here doesn't get used. Address is autodetected at startup.
#else
LcdDriver LcdDisplay::lcd;
#endif

// Constant strings used multiple times
static const char STR_Beer_[] PROGMEM = "Beer ";
static const char STR_Fridge_[] PROGMEM = "Fridge ";
static const char STR_Const_[] PROGMEM = "Const.";
static const char STR_Cool[] PROGMEM = "Cool";
static const char STR_Heat[] PROGMEM = "Heat";
static const char STR_ing_for[] PROGMEM = "ing for";
static const char STR_Wait_to_[] PROGMEM = "Wait to ";
static const char STR__time_left[] PROGMEM = " time left";
static const char STR_empty_string[] PROGMEM = "";

#if defined(ESP8266) || defined(ESP32)
bool toggleBacklight;
#endif


#ifndef min
#define min _min
#endif

#ifndef max
#define max _max
#endif

/**
 * \brief Invalid timestamp
 */
constexpr auto invalidTime = UINT16_MAX;

void LcdDisplay::init(){
#if defined(ESP8266) || defined(ESP32)
	toggleBacklight = false;
#endif
	stateOnDisplay = 0xFF; // set to unknown state to force update
	flags = LCD_FLAG_ALTERNATE_ROOM;
	lcd.init(); // initialize LCD
	lcd.begin(Config::Lcd::columns, Config::Lcd::lines);
	lcd.clear();
}

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

/**
 * \brief Print all temperatures on the LCD
 */
void LcdDisplay::printAllTemperatures(){
	// alternate between beer and room temp
	if (flags & LCD_FLAG_ALTERNATE_ROOM) {
		bool displayRoom = ((ticks.seconds()&0x08)==0) && !BREWPI_SIMULATE && tempControl.ambientSensor->isConnected();
		if (displayRoom ^ ((flags & LCD_FLAG_DISPLAY_ROOM)!=0)) {	// transition
			flags = displayRoom ? flags | LCD_FLAG_DISPLAY_ROOM : flags & ~LCD_FLAG_DISPLAY_ROOM;
			printStationaryText();
		}
	}

	printBeerTemp();
	printBeerSet();
	printFridgeTemp();
	printFridgeSet();
}

/**
 * \brief Set the display configuration flags
 *
 * Updates the display configuration and then forces a redraw.
 * @param newFlags - New flag values
 */
void LcdDisplay::setDisplayFlags(uint8_t newFlags) {
	flags = newFlags;
	printStationaryText();
	printAllTemperatures();
}


/**
 * Print beer temperature
 *
 * @see printTemperatureAt
 */
void LcdDisplay::printBeerTemp(){
	printTemperatureAt(6, 1, tempControl.getBeerTemp());
}


/**
 * Print beer target temperature
 *
 * @see printTemperatureAt
 */
void LcdDisplay::printBeerSet(){
	temperature beerSet = tempControl.getBeerSetting();
	printTemperatureAt(12, 1, beerSet);
}

void LcdDisplay::printFridgeTemp(){
	printTemperatureAt(6,2, flags & LCD_FLAG_DISPLAY_ROOM ?
		tempControl.ambientSensor->read() :
		tempControl.getFridgeTemp());
}

void LcdDisplay::printFridgeSet(){	
	temperature fridgeSet = tempControl.getFridgeSetting();	
	if(flags & LCD_FLAG_DISPLAY_ROOM) // beer setting is not active
		fridgeSet = INVALID_TEMP;
	printTemperatureAt(12, 2, fridgeSet);	
}


/**
 * \brief Print a temperature at a given coordinate
 *
 * @param x - LCD column
 * @param y - LCD row
 * @param temp - Temperature
 *
 * @see printTemperature
 */
void LcdDisplay::printTemperatureAt(uint8_t x, uint8_t y, temperature temp){
	lcd.setCursor(x,y);
	printTemperature(temp);
}


/**
 * \brief Print a temperature
 *
 * Invalid temps are drawn as `--.-` as a placeholder.  Valid temps are padded
 * to 5 chars wide.
 *
 * @param temp - Temperature to print
 */
void LcdDisplay::printTemperature(temperature temp){
	if (temp==INVALID_TEMP) {
		lcd.print_P(PSTR(" --.-"));
		return;
	}
	char tempString[9];
	tempToString(tempString, temp, 1 , 9);
	int8_t spacesToWrite = 5 - (int8_t) strlen(tempString);
	for(int8_t i = 0; i < spacesToWrite ;i++){
		lcd.write(' ');
	}
	lcd.print(tempString);
}

/**
 * \brief Print the stationary text on the lcd.
 */
void LcdDisplay::printStationaryText(){
	printAt_P(0, 0, PSTR("Mode"));
	printAt_P(0, 1, STR_Beer_);
	printAt_P(0, 2, (flags & LCD_FLAG_DISPLAY_ROOM) ?  PSTR("Room  ") : STR_Fridge_);
	printDegreeUnit(18, 1);
	printDegreeUnit(18, 2);
}

/**
 * \brief Print degree sign and temp unit
 *
 * @param x - LCD column
 * @param y - LCD row
 */
void LcdDisplay::printDegreeUnit(uint8_t x, uint8_t y){
	lcd.setCursor(x,y);
	lcd.write(0b11011111);
	lcd.write(tempControl.cc.tempFormat);
}

void LcdDisplay::printAt_P(uint8_t x, uint8_t y, const char* text){
	lcd.setCursor(x, y);
	lcd.print_P(text);
}


void LcdDisplay::printAt(uint8_t x, uint8_t y, char* text){
	lcd.setCursor(x, y);
	lcd.print(text);
}

// print mode on the right location on the first line, after "Mode   "
void LcdDisplay::printMode(){
	lcd.setCursor(7,0);
	// Factoring prints out of switch has negative effect on code size in this function
	switch(tempControl.getMode()){
    case Modes::fridgeConstant:
			lcd.print_P(STR_Fridge_);
			lcd.print_P(STR_Const_);
			break;
    case Modes::beerConstant:
			lcd.print_P(STR_Beer_);
			lcd.print_P(STR_Const_);
			break;
    case Modes::beerProfile:
			lcd.print_P(STR_Beer_);
			lcd.print_P(PSTR("Profile"));
			break;
    case Modes::off:
			lcd.print_P(PSTR("Off"));
			break;
    case Modes::test:
			lcd.print_P(PSTR("** Testing **"));
			break;
		default:
			lcd.print_P(PSTR("Invalid mode"));
			break;
	}
	lcd.printSpacesToRestOfLine();
}

// print the current state on the last line of the lcd
void LcdDisplay::printState(){
	uint16_t time = invalidTime;
	uint8_t state = tempControl.getDisplayState();
	if(state != stateOnDisplay){ //only print static text when state has changed
		stateOnDisplay = state;
		// Reprint state and clear rest of the line
		const char * part1 = STR_empty_string;
		const char * part2 = STR_empty_string;
		switch (state){
			case IDLE:
				part1 = PSTR("Idl");
				part2 = STR_ing_for;
				break;
			case WAITING_TO_COOL:
				part1 = STR_Wait_to_;
				part2 = STR_Cool;
				break;
			case WAITING_TO_HEAT:
				part1 = STR_Wait_to_;
				part2 = STR_Heat;
				break;
			case WAITING_FOR_PEAK_DETECT:
				part1 = PSTR("Waiting for peak");
				break;
			case COOLING:
				part1 = STR_Cool;
				part2 = STR_ing_for;
				break;
			case HEATING:
				part1 = STR_Heat;
				part2 = STR_ing_for;
				break;
			case COOLING_MIN_TIME:
				part1 = STR_Cool;
				part2 = STR__time_left;
				break;
			case HEATING_MIN_TIME:
				part1 = STR_Heat;
				part2 = STR__time_left;
				break;
			case DOOR_OPEN:
				part1 = PSTR("Door open");
				break;
			case STATE_OFF:
				part1 = PSTR("Temp. control OFF");
				break;
			default:
				part1 = PSTR("Unknown status!");
				break;
		}
		printAt_P(0, 3, part1);
		lcd.print_P(part2);		
		lcd.printSpacesToRestOfLine();
	}
	uint16_t sinceIdleTime = tempControl.timeSinceIdle();
	if(state==IDLE){
		time = 	min(tempControl.timeSinceCooling(), tempControl.timeSinceHeating());
	}
	else if(state==COOLING || state==HEATING){
		time = sinceIdleTime;
	}
	else if(state==COOLING_MIN_TIME){
		time = MIN_COOL_ON_TIME-sinceIdleTime;
	}
	
	else if(state==HEATING_MIN_TIME){
		time = MIN_HEAT_ON_TIME-sinceIdleTime;
	}
	else if(state == WAITING_TO_COOL || state == WAITING_TO_HEAT){
		time = tempControl.getWaitTime();
	}
	if(time != invalidTime){
		char timeString[10];
#if DISPLAY_TIME_HMS  // 96 bytes more space required. 
		unsigned int minutes = time/60;		
		unsigned int hours = minutes/60;
		int stringLength = sprintf_P(timeString, PSTR("%dh%02dm%02d"), hours, minutes%60, time%60);
		char * printString = timeString;
		if(!hours){
			printString = &timeString[2];
			stringLength = stringLength-2;
		}
		printAt(20-stringLength, 3, printString);
#else
		int stringLength = sprintf_P(timeString, STR_FMT_U, (unsigned int)time);
		printAt(20-stringLength, 3, timeString);
#endif		
	}
}


#ifdef ESP8266_WiFi

void LcdDisplay::printWiFiStartup(){
	toggleBacklight = false;  // Assuming we need this

	lcd.setCursor(0,0);

	lcd.print("Connect to this AP:");
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,1);
	lcd.print("AP Name: ");
	lcd.print(WIFI_SETUP_AP_NAME);
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,2);
	lcd.print("AP Pass: ");
	lcd.print(WIFI_SETUP_AP_PASS);
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,3);
	lcd.print("to configure device");
	lcd.printSpacesToRestOfLine();

	lcd.updateBacklight();
}


void LcdDisplay::printWiFi(){
	toggleBacklight = false;  // Assuming we need this

	lcd.setCursor(0,0);

	lcd.print("WiFi (mDNS) Name: ");
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,1);
	lcd.print(eepromManager.fetchmDNSName());
	lcd.print(".local");
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,2);
	lcd.print("IP Address: ");
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,3);
	lcd.print(WiFi.localIP());
	lcd.printSpacesToRestOfLine();

	lcd.updateBacklight();
}
#endif

void LcdDisplay::printEEPROMStartup(){
	toggleBacklight = false;  // Assuming we need this

	lcd.setCursor(0,0);
	// Factoring prints out of switch has negative effect on code size in this function
	lcd.print("Setting up EEPROM...");
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,1);
	lcd.print("Please wait. This");
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,2);
	lcd.print("can take 5+ minutes");
	lcd.printSpacesToRestOfLine();

	lcd.setCursor(0,3);
	lcd.print("for new installs.");
	lcd.printSpacesToRestOfLine();

	lcd.updateBacklight();
}





void LcdDisplay::clear() {
	lcd.clear();
}


#endif
