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
#include <stdio.h>
#include <stdlib.h>

#include "Brewpi.h"
#include "Config.h"
#include <stdarg.h>

#include "stddef.h"
#include "PiLink.h"

//#include "Version.h"
#include "TempControl.h"
#include "Display.h"
#include "JsonKeys.h"
#include "Ticks.h"
#include "Brewpi.h"
#include "EepromManager.h"
#include "EepromFormat.h"
#include "SettingsManager.h"
#include "Display.h" // Not sure why this is listed twice
#include "PiLinkHandlers.h"
#include "UI.h"
#include "Buzzer.h"
#include "ActuatorInterfaces.h"
#include "ActuatorMocks.h"
#include "Control.h"
#include "json_writer.h"

#ifdef ESP8266_WiFi_Control
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library
#endif

#ifdef ARDUINO
#ifndef ESP8266
#include "util/delay.h"
#endif
#endif

#if BREWPI_SIMULATE
#include "Simulator.h"
#endif


//#include <VM_DBG/VM_DBG.h>
 // Rename Serial to piStream, to abstract it for later platform independence

#ifdef ESP8266_WiFi
// Just use the serverClient object as it supports all the same functions as Serial
extern WiFiServer server;
extern WiFiClient serverClient;
#define piStream serverClient

#elif !defined(WIRING)
        StdIO stdIO;
        #define piStream stdIO
#else
// Not using ESP8266 WiFi
#define piStream Serial
#define SERIAL_READY(x) x
#endif

bool PiLink::firstPair;
char PiLink::printfBuff[PRINTF_BUFFER_SIZE];
#ifdef BUFFER_PILINK_PRINTS
String PiLink::printBuf;
#endif

void PiLink::init(void){
#ifndef ESP8266_WiFi
piStream.begin(57600);
#endif

#ifdef BUFFER_PILINK_PRINTS
printBuf.reserve(2048); // Reserve 2kb for our string (waaaay more than we need, and we have space on the ESP!)
printBuf = "";
#endif

}

#ifdef ESP8266
void formatStandardAnnotation(String &annotation, const char* str_1, const char* str_2, const char* str_3);
#endif

extern void handleReset();

// create a printf like interface to the Arduino Serial function. Format string stored in PROGMEM
void PiLink::print_P(const char *fmt, ... ){
	va_list args;
	va_start (args, fmt );
	vsnprintf_P(printfBuff, PRINTF_BUFFER_SIZE, fmt, args);
//	vsnprintf(printfBuff, PRINTF_BUFFER_SIZE, fmt, args);
	va_end (args);
#ifdef ESP8266_WiFi
	if (piStream && piStream.connected()) { // if WiFi client connected
#ifdef BUFFER_PILINK_PRINTS
		printBuf += printfBuff;
#else
		piStream.print(printfBuff);
#endif
	}
#else
	if (SERIAL_READY(piStream)) { // if Serial connected (on Leonardo)
		piStream.print(printfBuff);
	}
#endif
}

// create a printf like interface to the Arduino Serial function. Format string stored in RAM
void PiLink::print(char *fmt, ... ){
	va_list args;
	va_start (args, fmt );
	vsnprintf(printfBuff, PRINTF_BUFFER_SIZE, fmt, args);
	va_end (args);
#ifdef ESP8266_WiFi
	if (piStream && piStream.connected()) { // if WiFi client connected
#ifdef BUFFER_PILINK_PRINTS
		printBuf += printfBuff;
#else
		piStream.print(printfBuff);
#endif
	}
#else
	if (SERIAL_READY(piStream)) {
		piStream.print(printfBuff);
	}
#endif
}

#ifdef ESP8266
void PiLink::print(char out) {
#ifdef ESP8266_WiFi
	if (piStream && piStream.connected()) { // if WiFi client connected

#ifdef BUFFER_PILINK_PRINTS
		printBuf += out;
#else
		piStream.print(out);
#endif
	}
#else
	if (SERIAL_READY(piStream)) {
		piStream.print(out);
	}
#endif
}
#endif

void PiLink::printNewLine(){
#ifdef ESP8266_WiFi
	if (piStream && piStream.connected()) { // if WiFi client connected
#ifdef BUFFER_PILINK_PRINTS
		if (printBuf.length() > 0)
			piStream.print(printBuf);
		printBuf = "";
#endif
		piStream.println();
		yield();
		delay(50); // Give the controller enough time to transmit the full message
	}
#else
	if (SERIAL_READY(piStream)) {
		piStream.println();
	}
#endif
}

#if BREWPI_EEPROM_HELPER_COMMANDS
void PiLink::printNibble(uint8_t n)
{
	n &= 0xF;
	piStream.print((char)(n >= 10 ? n - 10 + 'A' : n + '0'));
}
#endif

// Trying to enforce that the only thing to talk to piStream is piLink
int PiLink::read() {
	return piStream.read();
}

void PiLink::receive(void){
	while (piStream.available() > 0) {
		char inByte = read();              
		switch(inByte){
		case ' ':
		case '\n':
		case '\r':
		case 'F': // Flash firmware not supported on ESP8266
#ifdef ESP8266_WiFi  // Control characters sent when establishing a telnet session 
		case 1:
		case 3:
		case 29:
		case 31:
		case '\'':
		case 251:
		case 253:
		case 255:
#endif
			break;

#if BREWPI_SIMULATE==1
		case 'y':
			parseJson(HandleSimulatorConfig);
			break;
		case 'Y':
			printSimulatorSettings();
			break;		
#endif					
#if BREWPI_BUZZER
		case 'A': // alarm on
			soundAlarm(true);
			break;
		case 'a': // alarm off
			soundAlarm(false);
			break;
#endif
		case 't': // temperatures requested
			printTemperatures();      
			break;		
		case 'C': // Set default constants
			tempControl.loadDefaultConstants();
			display.printStationaryText(); // reprint stationary text to update to right degree unit
			sendControlConstants(); // update script with new settings
			logInfo(INFO_DEFAULT_CONSTANTS_LOADED);
			break;
		case 'S': // Set default settings
			tempControl.loadDefaultSettings();
			sendControlSettings(); // update script with new settings
			logInfo(INFO_DEFAULT_SETTINGS_LOADED);
			break;
		case 's': // Control settings requested
			sendControlSettings();
			break;
		case 'c': // Control constants requested
			sendControlConstants();
			break;
		case 'v': // Control variables requested
			sendControlVariables();
			break;
		case 'n':
			// v version
			// s shield type
			// y: simulator			
			// b: board
#ifndef ESP8266
			print_P(PSTR("N:{\"v\":\"%S\",\"n\":%d,\"c\":\"%S\",\"s\":%d,\"y\":%d,\"b\":\"%c\",\"l\":\"%d\"}"), 
					PSTR(VERSION_STRING), 
					BUILD_NUMBER,
					PSTR(BUILD_NAME),
					BREWPI_STATIC_CONFIG, 
					BREWPI_SIMULATE, 
					BREWPI_BOARD,
					BREWPI_LOG_MESSAGES_VERSION);
#else
			// Doesn't appear to be support for %S for some reason. Replacing by breaking everything out.
			print("N:{\"v\":\"");
			print(VERSION_STRING); // TODO - Determine if BUILD_NUMBER is still sent (and remove if it isn't)
			print("\",\"n\":%d,\"c\":\"", BUILD_NUMBER);
			print(BUILD_NAME);
			print("\",\"s\":%d,\"y\":%d,\"b\":\"%c\",\"l\":\"%d\"}", 
				BREWPI_STATIC_CONFIG,  // Now getShieldVersion in 0.4.x
				BREWPI_SIMULATE,
				BREWPI_BOARD,
				BREWPI_LOG_MESSAGES_VERSION);
#endif
			printNewLine();
			break;
		case 'l': // Display content requested
			printResponse('L');  // was openListResponse('L');
			print('[');
			char stringBuffer[21];
			for(uint8_t i=0;i<4;i++){
				display.getLine(i, stringBuffer);
				print_P(PSTR("\"%s\""), stringBuffer);
				char close = (i<3) ? ',':']';
				print(close);
			}							
			printNewLine();						
			break;
		case 'j': // Receive settings as json
			receiveJson();
			break;

#if BREWPI_EEPROM_HELPER_COMMANDS
		case 'e': // dump contents of eeprom						
			openListResponse('E');
			for (uint16_t i=0; i<1024;) {
				if (i>0) {
					printNewLine();
					print(',');
				}
				piLink.print('\"');
				for (uint8_t j=0; j<64; j++) {
					uint8_t d = eepromAccess.readByte(i++);
					printNibble(d>>4);
					printNibble(d);
				}				
				piLink.print('\"');
			}
			closeListResponse();
			break;
#endif
			
		case 'E': // initialize eeprom
			eepromManager.initializeEeprom();
			logInfo(INFO_EEPROM_INITIALIZED);
			settingsManager.loadSettings();
			break;

		case 'd': // list devices in eeprom order
			openListResponse('d');
			deviceManager.listDevices();
			closeListResponse();
			break;

		case 'U': // update device		
			//printResponse('U'); // moved into function below, because installing devices can cause printing in between
			deviceManager.parseDeviceDefinition();
			//piLink.printNewLine();
			break;
			
		case 'h': // hardware query
			openListResponse('h');
			deviceManager.enumerateHardwareToStream();
			closeListResponse();
			break;

#ifdef ESP8266_WiFi_Control
		case 'w': // Reset WiFi settings
			WiFi.disconnect(true);
			break;
#endif

#ifdef ESP8266
		case 'b': // Toggle Backlight
			toggleBacklight = !toggleBacklight;
			break;
#endif

#if (BREWPI_DEBUG > 0)			
		case 'Z': // zap eeprom
			eepromManager.zapEeprom();
			logInfo(INFO_EEPROM_ZAPPED);
			break;
#endif

		case 'R': // reset 
            handleReset();
            break;
		default:
			logWarningInt(WARNING_INVALID_COMMAND, inByte);
		}
	}
}



#define COMPACT_SERIAL BREWPI_SIMULATE
#if COMPACT_SERIAL
	#define JSON_BEER_TEMP  "bt"
	#define JSON_BEER_SET	"bs"
	#define JSON_BEER_ANN	"ba"
	#define JSON_FRIDGE_TEMP "ft"
	#define JSON_FRIDGE_SET  "fs"
	#define JSON_FRIDGE_ANN  "fa"
	#define JSON_STATE		"s"
	#define JSON_TIME		"t"
	#define JSON_ROOM_TEMP  "rt"
	
	temperature beerTemp = -1, beerSet = -1, fridgeTemp = -1, fridgeSet = -1;
	double roomTemp = -1;
	uint8_t state = 0xFF;
	char* beerAnn; char* fridgeAnn;
	
	typedef char* PChar;
	inline bool changed(uint8_t &a, uint8_t b) { uint8_t c = a; a=b; return b!=c; }
	inline bool changed(temperature &a, temperature b) { temperature c = a; a=b; return b!=c; }
	inline bool changed(double &a, double b) { double c = a; a=b; return b!=c; }
	inline bool changed(PChar &a, PChar b) { PChar c = a; a=b; return b!=c; }
#else
	#define JSON_BEER_TEMP  "BeerTemp"
	#define JSON_BEER_SET	"BeerSet"
	#define JSON_BEER_ANN	"BeerAnn"
	#define JSON_FRIDGE_TEMP "FridgeTemp"
	#define JSON_FRIDGE_SET  "FridgeSet"
	#define JSON_FRIDGE_ANN  "FridgeAnn"
	#define JSON_STATE		"State"
	#define JSON_TIME		"Time"
	#define JSON_ROOM_TEMP  "RoomTemp"
	
	#define changed(a,b)  1
#endif

void PiLink::printTemperaturesJSON(const char * beerAnnotation, const char * fridgeAnnotation){
	printResponse('T');	


	temp_t t;
	t = tempControl.getBeerTemp();
	if (changed(beerTemp, t))
		sendJsonTemp(PSTR(JSON_BEER_TEMP), t);

	t = tempControl.getBeerSetting();
	if (changed(beerSet, t))
		sendJsonTemp(PSTR(JSON_BEER_SET), t);

	if (changed(beerAnn, beerAnnotation))
		sendJsonAnnotation(PSTR(JSON_BEER_ANN), beerAnnotation);

	t = tempControl.getFridgeTemp();
	if (changed(fridgeTemp, t))
		sendJsonTemp(PSTR(JSON_FRIDGE_TEMP), t);

	t = tempControl.getFridgeSetting();
	if (changed(fridgeSet, t))
		sendJsonTemp(PSTR(JSON_FRIDGE_SET), t);

	if (changed(fridgeAnn, fridgeAnnotation))
		sendJsonAnnotation(PSTR(JSON_FRIDGE_ANN), fridgeAnnotation);

	t = tempControl.getRoomTemp();
	if (changed(roomTemp, t))
		sendJsonTemp(PSTR(JSON_ROOM_TEMP), tempControl.getRoomTemp());

	if (changed(state, tempControl.getState()))
		sendJsonPair(PSTR(JSON_STATE), (uint8_t)tempControl.getState());

#if BREWPI_SIMULATE	
	printJsonName(PSTR(JSON_TIME));
	print_P(PSTR("%lu"), ticks.millis() / 1000);
#endif		
	sendJsonClose();
}

void PiLink::sendJsonAnnotation(const char* name, const char* annotation)
{
	printJsonName(name);
	const char* fmtAnn = annotation ? PSTR("\"%s\"") : PSTR("null");
	print_P(fmtAnn, annotation);
}

void PiLink::sendJsonTemp(const char* name, const temp_t & temp)
{
	char tempString[9];
	char * withoutSpaces = temp.toTempString(tempString, 2, 9, tempControl.cc.tempFormat, true);
	printJsonName(name);
	print(withoutSpaces);
}

void PiLink::printTemperatures(void){
	// print all temperatures with empty annotations
	printTemperaturesJSON(0, 0);
}

#ifndef ESP8266 // There is a bug with the ESP8266 which prevents this from working. Removing it so we aren't tempted to use it.
void PiLink::printBeerAnnotation(const char * annotation, ...) {
	char tempString[128]; // resulting string limited to 128 chars
	va_list args;
	// Using print_P for the Annotation fails. Arguments are not passed correctly. Use Serial directly as a work around.
	va_start(args, annotation);
	vsnprintf_P(tempString, 128, annotation, args);
	va_end(args);
	printTemperaturesJSON(tempString, 0);
}

void PiLink::printFridgeAnnotation(const char * annotation, ...) {
	char tempString[128]; // resulting string limited to 128 chars
	va_list args;
	// Using print_P for the Annotation fails. Arguments are not passed correctly. Use Serial directly as a work around.
	va_start(args, annotation);
	vsnprintf_P(tempString, 128, annotation, args);
	va_end(args);
	printTemperaturesJSON(0, tempString);
}
#endif

void PiLink::printResponse(char type) {

	print("%c:", type);
//	print(type);
//	print(':');
	firstPair = true;
}

void PiLink::openListResponse(char type) {
	printResponse(type);
	print('[');
}

void PiLink::closeListResponse() {
	print(']');
	printNewLine();
}


void PiLink::debugMessage(const char * message, ...){
	va_list args;
		
	//print 'D:' as prefix
	printResponse('D');
	
	// Using print_P for the Annotation fails. Arguments are not passed correctly. Use Serial directly as a work around.
	va_start (args, message );
	vsnprintf_P(printfBuff, PRINTF_BUFFER_SIZE, message, args);
	va_end (args);
	print(printfBuff);
	printNewLine();
}


void PiLink::sendJsonClose() {
	print("}");
	printNewLine();
}

// Send settings as JSON string
void PiLink::sendControlSettings(void){
	char tempString[12];
	printResponse('S');
	ControlSettings& cs = tempControl.cs;
	sendJsonPair(JSONKEY_mode, cs.mode);
	sendJsonPair(JSONKEY_beerSetting, cs.beerSetting.toTempString(tempString, 2, 12, tempControl.cc.tempFormat, true));
	sendJsonPair(JSONKEY_fridgeSetting, cs.fridgeSetting.toTempString(tempString, 2, 12, tempControl.cc.tempFormat, true));
	sendJsonClose();
}

// where the offset is relative to. This saves having to store a full 16-bit pointer.
// becasue the structs are static, we can only compute an offset relative to the struct (cc,cs,cv etc..)
// rather than offset from tempControl. 
uint8_t* jsonOutputBase;

void PiLink::jsonOutputUint8(const char* key, uint8_t offset) {
	piLink.sendJsonPair(key, *(jsonOutputBase+offset));
}

void PiLink::jsonOutputUint16(const char* key, uint8_t offset) {
	piLink.sendJsonPair(key, *((uint16_t*) (jsonOutputBase+offset)));
}

/**
 * outputs the temperature at the given offset from tempControl.cc.
 * The temperature is assumed to be an internal fixed point value.
 */
void PiLink::jsonOutputTempToString(const char* key, uint8_t offset) {
	char buf[12];
	piLink.sendJsonPair(key, ((temp_t*)(jsonOutputBase + offset))->toTempString(buf, 2, 12, tempControl.cc.tempFormat, true));
}

void PiLink::jsonOutputFixedPointToString(const char* key, uint8_t offset) {
	char buf[12];
	piLink.sendJsonPair(key, ((temp_t*)(jsonOutputBase + offset))->toString(buf, 2, 12));
}

void PiLink::jsonOutputTempDiffToString(const char* key, uint8_t offset) {
	char buf[12];
	piLink.sendJsonPair(key, ((temp_t*)(jsonOutputBase + offset))->toTempString(buf, 2, 12, tempControl.cc.tempFormat, false));
}

void PiLink::jsonOutputChar(const char* key, uint8_t offset) {
	piLink.sendJsonPair(key, *((char*)(jsonOutputBase + offset)));
}

typedef void (*JsonOutputCCHandler)(const char* key, uint8_t offset);

enum JsonOutputIndex {
	JOCC_UINT8=0,
	JOCC_TEMP_FORMAT=1,
	JOCC_FIXED_POINT=2,
	JOCC_TEMP_DIFF=3,
	JOCC_CHAR=4,
	JOCC_UINT16=5,
};

const PiLink::JsonOutputHandler PiLink::JsonOutputHandlers[] = {
	PiLink::jsonOutputUint8,
	PiLink::jsonOutputTempToString,
	PiLink::jsonOutputFixedPointToString,
	PiLink::jsonOutputTempDiffToString,
	PiLink::jsonOutputChar,
	PiLink::jsonOutputUint16,
};

#define JSON_OUTPUT_CC_MAP(name, fn) { JSONKEY_ ## name,  offsetof(ControlConstants, name), fn }
#define JSON_OUTPUT_CS_MAP(name, fn) { JSONKEY_ ## name,  offsetof(ControlSettings, name), fn }

const PiLink::JsonOutput PiLink::jsonOutputCCMap[] PROGMEM = {
	JSON_OUTPUT_CC_MAP(tempFormat, JOCC_CHAR),

	JSON_OUTPUT_CC_MAP(heater1_kp, JOCC_FIXED_POINT),
	JSON_OUTPUT_CC_MAP(heater1_ti, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(heater1_td, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(heater1_infilt, JOCC_UINT8),
	JSON_OUTPUT_CC_MAP(heater1_dfilt, JOCC_UINT8),

	JSON_OUTPUT_CC_MAP(heater2_kp, JOCC_FIXED_POINT),
	JSON_OUTPUT_CC_MAP(heater2_ti, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(heater2_td, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(heater2_infilt, JOCC_UINT8),
	JSON_OUTPUT_CC_MAP(heater2_dfilt, JOCC_UINT8),

	JSON_OUTPUT_CC_MAP(cooler_kp, JOCC_FIXED_POINT),
	JSON_OUTPUT_CC_MAP(cooler_ti, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(cooler_td, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(cooler_infilt, JOCC_UINT8),
	JSON_OUTPUT_CC_MAP(cooler_dfilt, JOCC_UINT8),

	JSON_OUTPUT_CC_MAP(beer2fridge_kp, JOCC_FIXED_POINT),
	JSON_OUTPUT_CC_MAP(beer2fridge_ti, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(beer2fridge_td, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(beer2fridge_infilt, JOCC_UINT8),
	JSON_OUTPUT_CC_MAP(beer2fridge_dfilt, JOCC_UINT8),
	JSON_OUTPUT_CC_MAP(beer2fridge_pidMax, JOCC_TEMP_DIFF),

	JSON_OUTPUT_CC_MAP(minCoolTime, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(minCoolIdleTime, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(heater1PwmPeriod, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(heater2PwmPeriod, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(coolerPwmPeriod, JOCC_UINT16),
	JSON_OUTPUT_CC_MAP(mutexDeadTime, JOCC_UINT16)
};

void PiLink::sendJsonValues(char responseType, const JsonOutput* /*PROGMEM*/ jsonOutputMap, uint8_t mapCount) {
	printResponse(responseType);
	while (mapCount-->0) {
		JsonOutput output;
		memcpy_P(&output, jsonOutputMap++, sizeof(output));
		JsonOutputHandlers[output.handlerOffset](output.key,output.offset);
	}
	sendJsonClose();
}

// Send control constants as JSON string. Might contain spaces between minus sign and number. Python will have to strip these
void PiLink::sendControlConstants(void){
	jsonOutputBase = (uint8_t*)&tempControl.cc;
	sendJsonValues('C', jsonOutputCCMap, sizeof(jsonOutputCCMap)/sizeof(jsonOutputCCMap[0]));	
}

/*
const PiLink::JsonOutput PiLink::jsonOutputCVMap[] PROGMEM = {
	JSON_OUTPUT_CV_MAP(beerDiff, JOCC_TEMP_DIFF),
	JSON_OUTPUT_CV_MAP(diffIntegral, JOCC_TEMP_DIFF),
	JSON_OUTPUT_CV_MAP(beerSlope, JOCC_TEMP_DIFF),
	JSON_OUTPUT_CV_MAP(p, JOCC_FIXED_POINT),
	JSON_OUTPUT_CV_MAP(i, JOCC_FIXED_POINT),
	JSON_OUTPUT_CV_MAP(d, JOCC_FIXED_POINT),
	JSON_OUTPUT_CV_MAP(estimatedPeak, JOCC_TEMP_FORMAT),
	JSON_OUTPUT_CV_MAP(negPeakEstimate, JOCC_TEMP_FORMAT),
	JSON_OUTPUT_CV_MAP(posPeakEstimate, JOCC_TEMP_FORMAT),
	JSON_OUTPUT_CV_MAP(negPeak, JOCC_TEMP_FORMAT),
	JSON_OUTPUT_CV_MAP(posPeak, JOCC_TEMP_FORMAT)	
};

// Send all control variables. Useful for debugging and choosing parameters
void PiLink::sendControlVariables(void){
	jsonOutputBase = (uint8_t*)&tempControl.cv;
	sendJsonValues('V', jsonOutputCVMap, sizeof(jsonOutputCVMap)/sizeof(jsonOutputCVMap[0]));
}
*/

// This function now sends the entire Control object as json using ESJ
void PiLink::sendControlVariables(void) {
	piStream.print('V');
	piStream.print(':');
	JSON::serial_producer<Control>::convert(control, piStream);
	piStream.println();
}


void PiLink::printJsonName(const char * name)
{
	printJsonSeparator();
	print('"');
	print_P(name);
	print("\":");
}

inline void PiLink::printJsonSeparator() {
	print(firstPair ? '{' : ',');	
	firstPair = false;
}

void PiLink::sendJsonPair(const char * name, const char * val){
	printJsonName(name);
	print_P(val);
}

void PiLink::sendJsonPair(const char * name, char val){
	printJsonName(name);
	print('"');
	print(val);
	print('"');
}

void PiLink::sendJsonPair(const char * name, uint16_t val){
	printJsonName(name);	
	print_P(PSTR("%u"), val);
}

void PiLink::sendJsonPair(const char * name, uint8_t val) {
	sendJsonPair(name, (uint16_t)val);
}

int readNext()
{
	uint8_t retries = 0;
	while (piStream.available()==0) {
#ifdef ESP8266
		// changing to delay as delayMicroseconds doesn't yield like delay does
		delay(1);
		yield();
#else
		_delay_us(100);
#endif
		retries++;
		if (retries >= 10000) {  // This was increased from 10 to 10000 in 0.4.x
			return -1;
		}
	}
	return piLink.read();		
}
/**
 * Parses a token from the piStream.
 * \return true if a token was parsed
 */
bool parseJsonToken(char* val) {
	uint8_t index = 0;
	val[0] = 0;
	bool result = true;
	for(;;) // get value
	{
		int character = readNext();
		if (index==29 || character == '}' || character==-1) {
			result = false;
			break;
		}
		if(character == ',' || character==':')  // end of value
			break;		
		if(character == ' ' || character == '"'){
			; // skip spaces and apostrophes
		}		
		else
			val[index++] = character;
	}
	val[index]=0; // null terminate string
	return result;	
}

void PiLink::parseJson(ParseJsonCallback fn, void* data) 
{
	char key[30];
	char val[30];
	*key = 0;
	*val = 0;
	bool next = true;
	// read first open brace	
	int c = readNext();		
	if (c!='{')
	{
		logErrorInt(ERROR_EXPECTED_BRACKET, c);
		return;
	}
	do {
		next = parseJsonToken(key) && parseJsonToken(val);
		if (val[0] && key[0])
			fn(key, val, data);
	} while (next);
}

void PiLink::receiveJson(void){

	parseJson(&processJsonPair, NULL);	
				
#if !BREWPI_SIMULATE	// this is quite an overhead and not needed for the simulator
	sendControlSettings();	// update script with new settings
	sendControlConstants();
#endif
	return;
}


static const char STR_WEB_INTERFACE[] PROGMEM = "in web interface";
static const char STR_TEMPERATURE_PROFILE[] PROGMEM = "by temp_t profile";
static const char STR_MODE[] PROGMEM = "Mode";
static const char STR_BEER_TEMP[] PROGMEM = "Beer temp";
static const char STR_FRIDGE_TEMP[] PROGMEM = "Fridge temp";
static const char STR_FMT_SET_TO[] PROGMEM = "%S set to %s %S";  // TODO - THis is changed to  PRINTF_PROGMEM " set to %s " PRINTF_PROGMEM; - Test that it still works

void PiLink::setMode(const char* val) {
	char mode = val[0];
	tempControl.setMode(mode, true);
#ifdef ESP8266
	String annotation = "";
	formatStandardAnnotation(annotation, STR_MODE, val, "in web interface");
	printTemperaturesJSON(0, annotation.c_str());
#else
	piLink.printFridgeAnnotation(STR_FMT_SET_TO, STR_MODE, val, STR_WEB_INTERFACE);
#endif
}

void PiLink::setBeerSetting(const char* val) {
#ifdef ESP8266
	String annotation = "";
#endif

	const char* source = NULL;
	temp_t newTemp;
	if (!newTemp.fromTempString(val, tempControl.cc.tempFormat, true)) {
		return; // could not parse value
	}

	if (tempControl.cs.mode == 'p') {
		if ((newTemp - tempControl.cs.beerSetting) > temp_t(0.2) || (tempControl.cs.beerSetting - newTemp) > temp_t(0.2)) { // this excludes gradual updates under 0.2 degrees
#ifdef ESP8266
			formatStandardAnnotation(annotation, STR_BEER_TEMP, val, "by temp_t profile");
#else
			source = STR_TEMPERATURE_PROFILE;
#endif
		}
	} else {
#ifdef ESP8266
		formatStandardAnnotation(annotation, STR_BEER_TEMP, val, "in web interface");
#else
		source = STR_WEB_INTERFACE;
#endif
	}
#ifdef ESP8266
	if (annotation.length() > 0)
		printTemperaturesJSON(annotation.c_str(), 0);
#else
	if (source)
		printBeerAnnotation(STR_FMT_SET_TO, STR_BEER_TEMP, val, source);
#endif
	tempControl.setBeerTemp(newTemp, true);
}

//There's some kind of strange bug with the ESP8266 (probably a memory issue) where if I pass STR_WEB_INTERFACE
//as str_3, everything dies. Same with using "printFridgeAnnotation" - Everything dies. 
#ifdef ESP8266
void formatStandardAnnotation(String &annotation, const char* str_1, const char* str_2, const char* str_3) {
	annotation += str_1;
	annotation += " set to ";
	annotation += str_2;
	annotation += " ";
	annotation += str_3;
}
#endif


void PiLink::setFridgeSetting(const char* val) {
	temp_t newTemp;
	if (!newTemp.fromTempString(val, tempControl.cc.tempFormat, true))
		return; // could not parse value

	if(tempControl.cs.mode == 'f'){
#ifdef ESP8266
		String annotation = "";
		formatStandardAnnotation(annotation, STR_FRIDGE_TEMP, val, "in web interface");
		printTemperaturesJSON(0, annotation.c_str());
#else
		printFridgeAnnotation(STR_FMT_SET_TO, STR_FRIDGE_TEMP, val, STR_WEB_INTERFACE);
#endif
	}
	tempControl.setFridgeTemp(newTemp, true);
}

void PiLink::setTempFormat(const char* val) {
	tempControl.cc.tempFormat = val[0];
	display.printStationaryText(); // reprint stationary text to update to right degree unit
	eepromManager.storeTempConstantsAndSettings();
}



void setFilter(const char* value, uint8_t* target) {
	uint16_t received;
	if (stringToUint16(&received, value)) {
		uint8_t setting = received;
		*target = setting;
		eepromManager.storeTempConstantsAndSettings(); // value parsed correctly
	}
}

void setStringToTemp(const char* value, temp_t* target) {
	if (target->fromTempString(value, tempControl.cc.tempFormat, true)) {
		eepromManager.storeTempConstantsAndSettings(); // value parsed correctly
	}
}

void setStringToTempDiff(const char* value, temp_t* target) {
	if (target->fromTempString(value, tempControl.cc.tempFormat, false)) {
		eepromManager.storeTempConstantsAndSettings(); // value parsed correctly
	}
}

void setStringToTempDiffLong(const char* value, temp_long_t* target) {
	if (target->fromTempString(value, tempControl.cc.tempFormat, false)) {
		eepromManager.storeTempConstantsAndSettings(); // value parsed correctly
	}
}

void setStringToFixed(const char* value, temp_long_t* target) {
	if (target->fromTempString(value, 'C', false)) {
		eepromManager.storeTempConstantsAndSettings(); // value parsed correctly
	}
}

void setStringToFixedLong(const char* value, temp_long_t* target) {
	if (target->fromTempString(value, 'C', false)) {
		eepromManager.storeTempConstantsAndSettings(); // value parsed correctly
	}
}

void setUint16(const char* value, uint16_t* target) {
	if (stringToUint16(target, value)) {
		eepromManager.storeTempConstantsAndSettings(); // value parsed correctly
	}
}
void setBool(const char* value, uint8_t* target) {
	bool result;
	if (stringToBool(&result, value)) {
		*target = result; // convert bool to uint8_t
		eepromManager.storeTempConstantsAndSettings();
	}
}



#define JSON_CONVERT(jsonKey, target, fn) { jsonKey, target, (JsonParserHandlerFn)&fn }

const PiLink::JsonParserConvert PiLink::jsonParserConverters[] PROGMEM = {
	JSON_CONVERT(JSONKEY_mode, NULL, setMode),
	JSON_CONVERT(JSONKEY_beerSetting, NULL, setBeerSetting),
	JSON_CONVERT(JSONKEY_fridgeSetting, NULL, setFridgeSetting),

	JSON_CONVERT(JSONKEY_tempFormat, NULL, setTempFormat),

	JSON_CONVERT(JSONKEY_heater1_kp, &tempControl.cc.heater1_kp, setStringToFixedLong),
	JSON_CONVERT(JSONKEY_heater1_ti, &tempControl.cc.heater1_ti, setUint16),
	JSON_CONVERT(JSONKEY_heater1_td, &tempControl.cc.heater1_td,setUint16),
	JSON_CONVERT(JSONKEY_heater1_infilt, &tempControl.cc.heater1_infilt, setFilter),
	JSON_CONVERT(JSONKEY_heater1_dfilt, &tempControl.cc.heater1_dfilt, setFilter),
	JSON_CONVERT(JSONKEY_heater2_kp, &tempControl.cc.heater2_kp, setStringToFixedLong),
	JSON_CONVERT(JSONKEY_heater2_ti, &tempControl.cc.heater2_ti, setUint16),
	JSON_CONVERT(JSONKEY_heater2_td, &tempControl.cc.heater2_td, setUint16),
	JSON_CONVERT(JSONKEY_heater2_infilt, &tempControl.cc.heater2_infilt, setFilter),
	JSON_CONVERT(JSONKEY_heater2_dfilt, &tempControl.cc.heater2_dfilt, setFilter),
	JSON_CONVERT(JSONKEY_cooler_kp, &tempControl.cc.cooler_kp, setStringToFixedLong),
	JSON_CONVERT(JSONKEY_cooler_ti, &tempControl.cc.cooler_ti, setUint16),
	JSON_CONVERT(JSONKEY_cooler_td, &tempControl.cc.cooler_td, setUint16),
	JSON_CONVERT(JSONKEY_cooler_infilt, &tempControl.cc.cooler_infilt, setFilter),
	JSON_CONVERT(JSONKEY_cooler_dfilt, &tempControl.cc.cooler_dfilt, setFilter),
	JSON_CONVERT(JSONKEY_beer2fridge_kp, &tempControl.cc.beer2fridge_kp, setStringToFixedLong),
	JSON_CONVERT(JSONKEY_beer2fridge_ti, &tempControl.cc.beer2fridge_ti, setUint16),
	JSON_CONVERT(JSONKEY_beer2fridge_td, &tempControl.cc.beer2fridge_td, setUint16),
	JSON_CONVERT(JSONKEY_beer2fridge_infilt, &tempControl.cc.beer2fridge_infilt, setFilter),
	JSON_CONVERT(JSONKEY_beer2fridge_dfilt, &tempControl.cc.beer2fridge_dfilt, setFilter),
	JSON_CONVERT(JSONKEY_beer2fridge_pidMax, &tempControl.cc.beer2fridge_pidMax, setStringToFixedLong),

	JSON_CONVERT(JSONKEY_minCoolTime, &tempControl.cc.minCoolTime, setUint16),
	JSON_CONVERT(JSONKEY_minCoolIdleTime, &tempControl.cc.minCoolIdleTime, setUint16),
	JSON_CONVERT(JSONKEY_heater1PwmPeriod, &tempControl.cc.heater1PwmPeriod, setUint16),
	JSON_CONVERT(JSONKEY_heater2PwmPeriod, &tempControl.cc.heater2PwmPeriod, setUint16),
	JSON_CONVERT(JSONKEY_coolerPwmPeriod, &tempControl.cc.coolerPwmPeriod, setUint16),
	JSON_CONVERT(JSONKEY_mutexDeadTime, &tempControl.cc.mutexDeadTime, setUint16)
	
};

void PiLink::processJsonPair(const char * key, const char * val, void* pv){
	logInfoStringString(INFO_RECEIVED_SETTING, key, val);
	
	for (uint8_t i=0; i<sizeof(jsonParserConverters)/sizeof(jsonParserConverters[0]); i++) {
		JsonParserConvert converter;
		memcpy_P(&converter, &jsonParserConverters[i], sizeof(converter));		
		//logDeveloper("Handling converter %d %s %S %d %d"), i, key, converter.key, converter.fn, converter.target);
		if (strcmp_P(key,converter.key) == 0) {
			//logDeveloper("Handling json key %s"), key);
			converter.fn(val, converter.target);
			return;
		}
	}					
	logWarning(WARNING_COULD_NOT_PROCESS_SETTING);
}

void PiLink::soundAlarm(bool active)
{
#if BREWPI_BUZZER
	buzzer.setActive(active);
#endif
}


#ifndef ARDUINO
void PiLink::print(char c) { piStream.print(c); }
#endif

