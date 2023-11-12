/*
 * Copyright 2019 John Beeler.
 *
 * This file has been added to BrewPi by the author noted above. It should be assumed to be available under the same
 * license terms as BrewPi, unless separate arrangements have been made with the author.
 *
 */

#include "Brewpi.h"
#include "BrewpiStrings.h"
#include <limits.h>
#include <stdint.h>
#include <string>

#include "Display.h"
#include "DisplayLcd.h"
#include "Menu.h"

#include "DisplayTFT.h"

#include "TempControl.h"
#include "TemperatureFormats.h"
#include "Pins.h"

#ifdef BREWPI_TFT


#ifdef ESP8266_WiFi

#if defined(ESP8266)
#include <ESP8266WiFi.h>  // For printing the IP address
#elif defined(ESP32)
#include <WiFi.h> // For printing the IP address
#else
#error "Invalid chipset!"
#endif

#endif


#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//#include <XPT2046_Touchscreen.h>


bool toggleBacklight;


#ifndef min
#define min _min
#endif

#ifndef max
#define max _max
#endif


LcdDisplay::LcdDisplay() {
    // Initialize the display device -- we can reinitialize later if needed
    tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
    // We can't do much more as this object gets created at boot
}

LcdDisplay::~LcdDisplay() {
    delete tft;
}


void LcdDisplay::print_layout() {

    // Print the lines for the basic layout
    tft->drawLine(0, BEER_NAME_LINE_Y, 320, BEER_NAME_LINE_Y, ILI9341_WHITE);  // Bar beneath beer name/set points
    tft->drawLine(0, ACTUAL_SET_LINE_Y, 320, ACTUAL_SET_LINE_Y, ILI9341_DARKGREY);  // Bar between set/measured points
    tft->drawLine(0, MIDDLE_BAR_Y_END, 320, MIDDLE_BAR_Y_END, ILI9341_WHITE);  // Bar beneath set/measured points
    tft->drawLine(MIDDLE_BAR_X, BEER_NAME_LINE_Y, MIDDLE_BAR_X, MIDDLE_BAR_Y_END, ILI9341_WHITE);  // Middle bar

    tft->drawLine(GRAVITY_LINE_X, GRAVITY_LINE_START_Y, GRAVITY_LINE_X, 240, ILI9341_WHITE);  // Gravity bar


    // Print the headers
    tft->setTextSize(HEADER_FONT_SIZE);
    tft->setCursor(FRIDGE_HEADER_START_X, HEADER_START_Y);
    tft->print((flags & LCD_FLAG_DISPLAY_ROOM) ?  "Room  " : "Fridge");


    tft->setTextSize(HEADER_FONT_SIZE);
    tft->setCursor(BEER_HEADER_START_X, HEADER_START_Y);
    tft->print("Beer");

    tft->setTextSize(SET_HEADER_FONT_SIZE);
    tft->setCursor(FRIDGE_SET_HEADER_START_X, SET_HEADER_START_Y);
    tft->print("Set");
    tft->setTextSize(SET_HEADER_FONT_SIZE);
    tft->setCursor(BEER_SET_HEADER_START_X, SET_HEADER_START_Y);
    tft->print("Set");

    // Print the "Now Fermenting" mesage
    tft->setTextSize(BEER_NAME_FONT_SIZE);
    tft->setCursor(BEER_NAME_START_X, BEER_NAME_START_Y);
//    tft->print("Current Beer: Belgian Beer Series - The Saisoning");

}

void LcdDisplay::init(){
    toggleBacklight = false;
    stateOnDisplay = 0xFF; // set to unknown state to force update
    flags = LCD_FLAG_ALTERNATE_ROOM;  // TODO - Test with a room sensor to see what happens

    tft->begin();
    if (extendedSettings.invertTFT)
        tft->setRotation(3);
    else
        tft->setRotation(1);
    tft->fillScreen(ILI9341_BLACK);
    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);

#if defined(TFT_BACKLIGHT)
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, HIGH);
#endif

}

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

//print all temperatures on the LCD
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

void LcdDisplay::setDisplayFlags(uint8_t newFlags) {
    flags = newFlags;
    printStationaryText();
    printAllTemperatures();
}



void LcdDisplay::printBeerTemp(){
    printTemperatureAt(BEER_TEMP_START_X, MAIN_TEMP_START_Y, MAIN_TEMP_FONT_SIZE, tempControl.getBeerTemp());
}

void LcdDisplay::printBeerSet(){
//    temperature beerSet = tempControl.getBeerSetting();
    printTemperatureAt(BEER_SET_START_X, SET_TEMP_START_Y, SET_TEMP_FONT_SIZE, tempControl.getBeerSetting());
}

void LcdDisplay::printFridgeTemp(){
    printTemperatureAt(FRIDGE_TEMP_START_X, MAIN_TEMP_START_Y, MAIN_TEMP_FONT_SIZE, flags & LCD_FLAG_DISPLAY_ROOM ?
                            tempControl.ambientSensor->read() :
                            tempControl.getFridgeTemp());
}

void LcdDisplay::printFridgeSet(){
    temperature fridgeSet = tempControl.getFridgeSetting();
    if(flags & LCD_FLAG_DISPLAY_ROOM) // beer setting is not active
        fridgeSet = INVALID_TEMP;
    printTemperatureAt(FRIDGE_SET_START_X, SET_TEMP_START_Y, SET_TEMP_FONT_SIZE, fridgeSet);
}

void LcdDisplay::printTemperatureAt(uint8_t x, uint8_t y, uint8_t font_size, temperature temp){
    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    //tft->setTextSize(font_size);
    //clearForText(x, y, ILI9341_BLACK, font_size, 4);  // TODO - Determine if I want this to be 4 or 5 characters

    tft->setCursor(x,y);
    printTemperature(temp, font_size);
}


void LcdDisplay::printTemperature(temperature temp, uint8_t font_size){
    tft->setTextSize(font_size);

    if (temp==INVALID_TEMP) {
        tft->print("  --.-");
        return;
    }


    char tempString[9];
    char tempBuf[9];
    tempToString(tempString, temp, 1 , 9);

    // Pad the width 
    switch(strlen(tempString)) {
        case 5:
            snprintf(tempBuf, 9, " %s %c", tempString, tempControl.cc.tempFormat);
            break;
        case 4:
            snprintf(tempBuf, 9, "  %s %c", tempString, tempControl.cc.tempFormat);
            break;
        case 3:
            snprintf(tempBuf, 9, "   %s %c", tempString, tempControl.cc.tempFormat);
            break;
        default:
            snprintf(tempBuf, 9, "%s %c", tempString, tempControl.cc.tempFormat);
            break;
    }

    tft->print(tempBuf);
}

//print the stationary text on the lcd.
void LcdDisplay::printStationaryText(){
    print_layout();
}

void LcdDisplay::printMode(){
    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(MODE_FONT_SIZE);
    clearForText(MODE_START_X, MODE_START_Y, ILI9341_BLACK, MODE_FONT_SIZE, 21);

    tft->setCursor(MODE_START_X, MODE_START_Y);
    tft->print("Mode: ");

    switch(tempControl.getMode()){
        case Modes::fridgeConstant:
            tft->print("Fridge Constant");
            break;
        case Modes::beerConstant:
            tft->print("Beer Constant  ");
            break;
        case Modes::beerProfile:
            tft->print("Beer Profile   ");
            break;
        case Modes::off:
            tft->print("Off            ");
            break;
        case Modes::test:
            tft->print("** Testing **  ");
            break;
        default:
            tft->print("Invalid Mode   ");
            break;
    }
}

void LcdDisplay::printIPAddressInfo(){
    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(IP_ADDRESS_FONT_SIZE);
    clearForText(IP_ADDRESS_START_X, IP_ADDRESS_START_Y, ILI9341_BLACK, MODE_FONT_SIZE, 21);

    tft->setCursor(IP_ADDRESS_START_X, IP_ADDRESS_START_Y);
    tft->print("IP Address: ");

    if(WiFi.isConnected()) {
        tft->print(WiFi.localIP());
    } else {
        tft->print("Disconnected");
    }

}


uint8_t LcdDisplay::printTime(uint16_t time) {
    if(time == UINT16_MAX)
        return 0;

    char timeString[10];
#if DISPLAY_TIME_HMS  // 96 bytes more space required.
    unsigned int minutes = time/60;
    unsigned int hours = minutes/60;
    int stringLength = sprintf_P(timeString, PSTR("%dh%02dm%02ds"), hours, minutes%60, time%60);
    char * printString = timeString;
    if(!hours){
        printString = &timeString[2];
        stringLength = stringLength-2;
    }
//        printAt(20-stringLength, 3, printString);
    tft->print(printString);
    return stringLength;

#else
#warning "This has not been tested"
    int stringLength = sprintf_P(timeString, STR_FMT_U, (unsigned int)time);
    printAt(20-stringLength, 3, timeString);
    tft->print(timeString);
    return stringLength;
#endif
}

// print the current state on the last line of the lcd
void LcdDisplay::printState(){
    uint16_t time = UINT16_MAX; // init to max
    uint8_t state = tempControl.getDisplayState();
    uint8_t printed_chars = 8;

    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(STATUS_FONT_SIZE);

    tft->setCursor(STATUS_START_X, STATUS_START_Y);
    tft->print("Status: ");

    // For the TFT, always reprint the state text
    switch (state){
        case IDLE:
            tft->print("Idling");
            printed_chars += 6;
            break;
        case WAITING_TO_COOL:
            tft->print("Waiting to cool");
            printed_chars += 15;
            break;
        case WAITING_TO_HEAT:
            tft->print("Waiting to heat");
            printed_chars += 15;
            break;
        case WAITING_FOR_PEAK_DETECT:
            tft->print("Waiting for peak");
            printed_chars += 16;
            break;
        case COOLING:
            tft->print("Cooling");
            printed_chars += 7;
            break;
        case HEATING:
            tft->print("Heating");
            printed_chars += 7;
            break;
        case COOLING_MIN_TIME:
            tft->print("Cooling");
            printed_chars += 7;
            break;
        case HEATING_MIN_TIME:
            tft->print("Heating");
            printed_chars += 7;
            break;
        case DOOR_OPEN:
            tft->print("Door open");
            printed_chars += 9;
            break;
        case STATE_OFF:
            tft->print("Off");
            printed_chars += 3;
            break;
        default:
            tft->print("Unknown status!");
            printed_chars += 15;
            break;
    }

    uint16_t sinceIdleTime = tempControl.timeSinceIdle();
    if(state==IDLE){
        tft->print(" for ");
        printed_chars += 15;
        time = 	min(tempControl.timeSinceCooling(), tempControl.timeSinceHeating());
        printed_chars += printTime(time);
    } else if(state==COOLING || state==HEATING){
        tft->print(" for ");
        printed_chars += 5;
        time = sinceIdleTime;
        printed_chars += printTime(time);
    } else if(state==COOLING_MIN_TIME){
        tft->print(" time left ");
        printed_chars += 5;
        time = tempControl.getMinCoolOnTime()-sinceIdleTime;
        printed_chars += printTime(time);
    } else if(state==HEATING_MIN_TIME){
        tft->print(" time left ");
        printed_chars += 11;
        time = tempControl.getMinHeatOnTime()-sinceIdleTime;
        printed_chars += printTime(time);
    } else if(state == WAITING_TO_COOL || state == WAITING_TO_HEAT){
        tft->print(" ");
        printed_chars += 1;
        time = tempControl.getWaitTime();
        printed_chars += printTime(time);
    }

    // Because of the way we're updating the display, we need to clear out everything to the right of the status
    // string
    for (int i = printed_chars; i < 33; ++i) {
        tft->print(" ");
    }

}


#ifdef ESP8266_WiFi
void LcdDisplay::printWiFi(){
    clear();

    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(WIFI_FONT_SIZE);

    tft->setCursor(0, 0);

    tft->println("mDNS Name: ");
    tft->print(eepromManager.fetchmDNSName());
    tft->println(".local");

    tft->println(" ");

    tft->println("IP Address: ");

    tft->println(WiFi.localIP());
}

void LcdDisplay::printWiFiStartup(){
    clear();

    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(WIFI_FONT_SIZE);

    tft->setCursor(0, 0);

    tft->println("Using a phone, laptop, or ");
    tft->println("other device connect to ");
    tft->println("the following WiFi network");
    tft->println("to configure this BrewPi");
    tft->println("controller:");
    tft->println("");

    tft->print("AP Name: ");
    tft->println(WIFI_SETUP_AP_NAME);

    tft->print("AP Pass: ");
    tft->println(WIFI_SETUP_AP_PASS);
}

void LcdDisplay::printWiFiConnect(){
    clear();

    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(WIFI_FONT_SIZE);

    tft->setCursor(0, 0);

    tft->println("Attempting to connect to ");
    tft->println("WiFi. This may take up to ");
    tft->println("one minute.");
}

#endif

#ifdef HAS_BLUETOOTH
void LcdDisplay::printBluetoothStartup(){
    toggleBacklight = false;  // Assuming we need this

    clear();

    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(WIFI_FONT_SIZE);

    tft->setCursor(0, 0);

    tft->println("Searching for Bluetooth");
    tft->println("devices. ");
    tft->println("");
    tft->println("This scan will take about");
    tft->println("15 seconds. Booting will");
    tft->println("continue once complete.");
}

void LcdDisplay::printGravity(){
    tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft->setTextSize(GRAVITY_HEADER_FONT_SIZE);
    clearForText(GRAVITY_START_X, GRAVITY_HEADER_START_Y, ILI9341_BLACK, GRAVITY_HEADER_FONT_SIZE, 7);
    clearForText(GRAVITY_START_X, GRAVITY_START_Y, ILI9341_BLACK, GRAVITY_HEADER_FONT_SIZE, 5);

    tft->setCursor(GRAVITY_START_X, GRAVITY_HEADER_START_Y);
    tft->print("Gravity");


    // Print the Gravity
    double grav = 80.0 / 1000.0;



    char grav_text[6];
    snprintf(grav_text, 6, "%05.3f", grav);
    tft->setTextSize(GRAVITY_FONT_SIZE);
    tft->setCursor(GRAVITY_START_X, GRAVITY_START_Y);
    tft->print(grav_text);
}
#endif


void LcdDisplay::clear() {
    tft->fillScreen(ILI9341_BLACK);
}

void LcdDisplay::clearForText(uint8_t start_x, uint8_t start_y, uint16_t color, uint8_t font_size, uint8_t characters) {
//    uint8_t width = (font_size * characters * 5) + (font_size * (characters-1) * 4);
//    uint8_t height = font_size * 7;

//    tft->fillRect(start_x, start_y, width, height, color);
}



/*
Mode   Beer Const.  
Beer   52.1  52.0 °F
Fridge 58.7  44.9 °F
Cooling for    04m01
*/

std::string getline_temp_string(temperature temp) {
    if (temp==INVALID_TEMP) {
        std::string str(" --.-");
        return str;
    }

    char tempString[9];
    tempToString(tempString, temp, 1 , 9);
    std::string str(tempString);
    while(str.length() < 5)
        str.insert(0, " ");
    return str;
}

void LcdDisplay::getLine(uint8_t lineNumber, char * buffer) {

    char line_buf[25];
    const char degree_symbol = 248;

    switch(lineNumber) {
        case 0:
            {
                std::string line;
                switch(tempControl.getMode()) {
                    case Modes::fridgeConstant:
                        line = "Fridge Const.";
                        break;
                    case Modes::beerConstant:
                        line = "Beer Const.";
                        break;
                    case Modes::beerProfile:
                        line = "Beer Profile";
                        break;
                    case Modes::off:
                        line = "Off";
                        break;
                    case Modes::test:
                        line = "** Testing **";
                        break;
                    default:
                        line = "Invalid Mode";
                        break;
                }
                snprintf(line_buf, 25, "Mode   %s", line.c_str());
                break;
            }
        case 1:
            {
                snprintf(line_buf, 25, "Beer  %s %s %c%c", getline_temp_string(tempControl.getBeerTemp()).c_str(), getline_temp_string(tempControl.getBeerSetting()).c_str(), degree_symbol, tempControl.cc.tempFormat);
                break;
            }
        case 2:
            {
                snprintf(line_buf, 25, "Fridge%s %s %c%c", getline_temp_string(tempControl.getFridgeTemp()).c_str(), getline_temp_string(tempControl.getFridgeSetting()).c_str(), degree_symbol, tempControl.cc.tempFormat);
                break;
            }
        case 3:
            {
                std::string line;
                line = "";

                uint16_t time = UINT16_MAX; // init to max
                uint8_t state = tempControl.getDisplayState();


                switch (state){
                    case IDLE:
                        line = "Idling";
                        break;
                    case WAITING_TO_COOL:
                        line = "Wait to cool";
                        break;
                    case WAITING_TO_HEAT:
                        line = "Wait to heat";
                        break;
                    case WAITING_FOR_PEAK_DETECT:
                        line = "Wait for peak";
                        break;
                    case COOLING:
                        line = "Cooling";
                        break;
                    case HEATING:
                        line = "Heating";
                        break;
                    case COOLING_MIN_TIME:
                        line = "Cooling";
                        break;
                    case HEATING_MIN_TIME:
                        line = "Heating";
                        break;
                    case DOOR_OPEN:
                        line = "Door open";
                        break;
                    case STATE_OFF:
                        line = "Temp control OFF";
                        break;
                    default:
                        line = "Unknown status!";
                        break;
                }

                uint16_t sinceIdleTime = tempControl.timeSinceIdle();
                if(state==IDLE){
                    line += " for ";
                    time = 	min(tempControl.timeSinceCooling(), tempControl.timeSinceHeating());
                }
                else if(state==COOLING || state==HEATING){
                    line += " for ";
                    time = sinceIdleTime;
                }
                else if(state==COOLING_MIN_TIME){
                    line += " time left ";
                    time = tempControl.getMinCoolOnTime()-sinceIdleTime;
                }

                else if(state==HEATING_MIN_TIME){
                    line += " time left ";
                    time = tempControl.getMinHeatOnTime()-sinceIdleTime;
                }
                else if(state == WAITING_TO_COOL || state == WAITING_TO_HEAT){
                    line += " ";
                    time = tempControl.getWaitTime();
                } 
                if(time != UINT16_MAX) {
                    char timeString[10];
                    unsigned int minutes = time/60;
                    unsigned int hours = minutes/60;
                    int stringLength = sprintf_P(timeString, PSTR("%dh%02dm%02d"), hours, minutes%60, time%60);
                    char * printString = timeString;
                    if(!hours){
                        printString = &timeString[2];
                        stringLength = stringLength-2;
                    }
        //        printAt(20-stringLength, 3, printString);
                    line += printString;
                }
                snprintf(line_buf, 25, "%s", line.c_str());

                break;
            }

        default:
            {
                snprintf(line_buf, 25, "Invalid line requested.");
                break;
            }
    }

    for (uint8_t i = 0; i <= Config::Lcd::columns; i++) {
        char c = ' ';
        if(i < strlen(line_buf))
            c = line_buf[i];
        buffer[i] = (c == 0b11011111) ? 0xB0 : c;
    }
    buffer[Config::Lcd::columns + 1] = '\0'; // NULL terminate string


}

#endif
