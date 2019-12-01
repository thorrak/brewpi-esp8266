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


uint8_t LcdDisplay::stateOnDisplay;
uint8_t LcdDisplay::flags;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);


bool toggleBacklight;


#ifndef min
#define min _min
#endif

#ifndef max
#define max _max
#endif



void LcdDisplay::print_layout(void) {

    // Print the lines for the basic layout
    tft.drawLine(0, BEER_NAME_LINE_Y, 320, BEER_NAME_LINE_Y, ILI9341_WHITE);  // Bar beneath beer name/set points
    tft.drawLine(0, ACTUAL_SET_LINE_Y, 320, ACTUAL_SET_LINE_Y, ILI9341_DARKGREY);  // Bar between set/measured points
    tft.drawLine(0, MIDDLE_BAR_Y_END, 320, MIDDLE_BAR_Y_END, ILI9341_WHITE);  // Bar beneath set/measured points
    tft.drawLine(MIDDLE_BAR_X, BEER_NAME_LINE_Y, MIDDLE_BAR_X, MIDDLE_BAR_Y_END, ILI9341_WHITE);  // Middle bar

    tft.drawLine(GRAVITY_LINE_X, GRAVITY_LINE_START_Y, GRAVITY_LINE_X, 240, ILI9341_WHITE);  // Gravity bar


    // Print the headers
    tft.setTextSize(HEADER_FONT_SIZE);
    tft.setCursor(FRIDGE_HEADER_START_X, HEADER_START_Y);
    tft.print((flags & LCD_FLAG_DISPLAY_ROOM) ?  "Room" : "Fridge");


    tft.setTextSize(HEADER_FONT_SIZE);
    tft.setCursor(BEER_HEADER_START_X, HEADER_START_Y);
    tft.print("Beer");

    tft.setTextSize(SET_HEADER_FONT_SIZE);
    tft.setCursor(FRIDGE_SET_HEADER_START_X, SET_HEADER_START_Y);
    tft.print("Set");
    tft.setTextSize(SET_HEADER_FONT_SIZE);
    tft.setCursor(BEER_SET_HEADER_START_X, SET_HEADER_START_Y);
    tft.print("Set");

    tft.setTextSize(GRAVITY_HEADER_FONT_SIZE);
    tft.setCursor(GRAVITY_START_X, GRAVITY_HEADER_START_Y);
//    tft.print("Gravity");



    // Print the "Now Fermenting" mesage
    tft.setTextSize(BEER_NAME_FONT_SIZE);
    tft.setCursor(BEER_NAME_START_X, BEER_NAME_START_Y);
//    tft.print("Current Beer: Belgian Beer Series - The Saisoning");


    // Print the Gravity
    tft.setTextSize(GRAVITY_FONT_SIZE);
    tft.setCursor(GRAVITY_START_X, GRAVITY_START_Y);
//    tft.print("1.084");


}

void LcdDisplay::init(void){
#if defined(ESP8266) || defined(ESP32)
    toggleBacklight = false;
#endif
    stateOnDisplay = 0xFF; // set to unknown state to force update
    flags = LCD_FLAG_ALTERNATE_ROOM;  // TODO - Test with a room sensor to see what happens
//    lcd.init(); // initialize LCD
//    lcd.begin(20, 4);
//    lcd.clear();

    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

#if defined(TFT_BACKLIGHT)
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, HIGH);
#endif

}

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

//print all temperatures on the LCD
void LcdDisplay::printAllTemperatures(void){
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



void LcdDisplay::printBeerTemp(void){
    printTemperatureAt(BEER_TEMP_START_X, MAIN_TEMP_START_Y, MAIN_TEMP_FONT_SIZE, tempControl.getBeerTemp());
}

void LcdDisplay::printBeerSet(void){
//    temperature beerSet = tempControl.getBeerSetting();
    printTemperatureAt(BEER_SET_START_X, SET_TEMP_START_Y, SET_TEMP_FONT_SIZE, tempControl.getBeerSetting());
}

void LcdDisplay::printFridgeTemp(void){
    printTemperatureAt(FRIDGE_TEMP_START_X, MAIN_TEMP_START_Y, MAIN_TEMP_FONT_SIZE, flags & LCD_FLAG_DISPLAY_ROOM ?
                            tempControl.ambientSensor->read() :
                            tempControl.getFridgeTemp());
}

void LcdDisplay::printFridgeSet(void){
    temperature fridgeSet = tempControl.getFridgeSetting();
    if(flags & LCD_FLAG_DISPLAY_ROOM) // beer setting is not active
        fridgeSet = INVALID_TEMP;
    printTemperatureAt(FRIDGE_SET_START_X, SET_TEMP_START_Y, SET_TEMP_FONT_SIZE, fridgeSet);
}

void LcdDisplay::printTemperatureAt(uint8_t x, uint8_t y, uint8_t font_size, temperature temp){
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    //tft.setTextSize(font_size);
    //clearForText(x, y, ILI9341_BLACK, font_size, 4);  // TODO - Determine if I want this to be 4 or 5 characters

    tft.setCursor(x,y);
    printTemperature(temp, font_size);
}


void LcdDisplay::printTemperature(temperature temp, uint8_t font_size){
    tft.setTextSize(font_size);

    if (temp==INVALID_TEMP) {
        tft.print("--.-");
        return;
    }


    char tempString[9];
    tempToString(tempString, temp, 1 , 9);
    // TODO - Determine if I want to pad single digits
//    int8_t spacesToWrite = 5 - (int8_t) strlen(tempString);
//    for(int8_t i = 0; i < spacesToWrite ;i++){
//        lcd.write(' ');
//    }
//    lcd.print(tempString);
    tft.print(tempString);
}

//print the stationary text on the lcd.
void LcdDisplay::printStationaryText(void){
//    printAt_P(0, 0, PSTR("Mode"));
//    printAt_P(0, 1, STR_Beer_);
//    printAt_P(0, 2, (flags & LCD_FLAG_DISPLAY_ROOM) ?  PSTR("Room  ") : STR_Fridge_);
//    printDegreeUnit(18, 1);
//    printDegreeUnit(18, 2);

    print_layout();
}

//print degree sign + temp unit
void LcdDisplay::printDegreeUnit(uint8_t x, uint8_t y){
//    lcd.setCursor(x,y);
//    lcd.write(0b11011111);
//    lcd.write(tempControl.cc.tempFormat);
}


// print mode on the right location on the first line, after "Mode   "
void LcdDisplay::printMode(void){
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(MODE_FONT_SIZE);
    clearForText(MODE_START_X, MODE_START_Y, ILI9341_BLACK, MODE_FONT_SIZE, 21);

    tft.setCursor(MODE_START_X, MODE_START_Y);
    tft.print("Mode: ");

    switch(tempControl.getMode()){
        case MODE_FRIDGE_CONSTANT:
            tft.print("Fridge Constant");
            break;
        case MODE_BEER_CONSTANT:
            tft.print("Beer Constant");
            break;
        case MODE_BEER_PROFILE:
            tft.print("Beer Profile");
            break;
        case MODE_OFF:
            tft.print("Off");
            break;
        case MODE_TEST:
            tft.print("** Testing **");
            break;
        default:
            tft.print("Invalid Mode");
            break;
    }

}

// print the current state on the last line of the lcd
void LcdDisplay::printState(void){
    uint16_t time = UINT16_MAX; // init to max
    uint8_t state = tempControl.getDisplayState();
    uint8_t printed_chars = 8;

    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(STATUS_FONT_SIZE);

    tft.setCursor(STATUS_START_X, STATUS_START_Y);
    tft.print("Status: ");

    // For the TFT, always reprint the state text
    switch (state){
        case IDLE:
            tft.print("Idling");
            printed_chars += 6;
            break;
        case WAITING_TO_COOL:
            tft.print("Waiting to cool");
            printed_chars += 15;
            break;
        case WAITING_TO_HEAT:
            tft.print("Waiting to heat");
            printed_chars += 15;
            break;
        case WAITING_FOR_PEAK_DETECT:
            tft.print("Waiting for peak");
            printed_chars += 16;
            break;
        case COOLING:
            tft.print("Cooling");
            printed_chars += 7;
            break;
        case HEATING:
            tft.print("Heating");
            printed_chars += 7;
            break;
        case COOLING_MIN_TIME:
            tft.print("Cooling");
            printed_chars += 7;
            break;
        case HEATING_MIN_TIME:
            tft.print("Heating");
            printed_chars += 7;
            break;
        case DOOR_OPEN:
            tft.print("Door open");
            printed_chars += 9;
            break;
        case STATE_OFF:
            tft.print("Off");
            printed_chars += 3;
            break;
        default:
            tft.print("Unknown status!");
            printed_chars += 15;
            break;
    }

    uint16_t sinceIdleTime = tempControl.timeSinceIdle();
    if(state==IDLE){
        tft.print(" for ");
        printed_chars += 15;
        time = 	min(tempControl.timeSinceCooling(), tempControl.timeSinceHeating());
    }
    else if(state==COOLING || state==HEATING){
        tft.print(" for ");
        printed_chars += 5;
        time = sinceIdleTime;
    }
    else if(state==COOLING_MIN_TIME){
        tft.print(" time left ");
        printed_chars += 5;
        time = MIN_COOL_ON_TIME-sinceIdleTime;
    }

    else if(state==HEATING_MIN_TIME){
        tft.print(" time left ");
        printed_chars += 11;
        time = MIN_HEAT_ON_TIME-sinceIdleTime;
    }
    else if(state == WAITING_TO_COOL || state == WAITING_TO_HEAT){
        tft.print(" ");
        printed_chars += 1;
        time = tempControl.getWaitTime();
    }
    if(time != UINT_MAX){
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
//        printAt(20-stringLength, 3, printString);
        tft.print(printString);
        printed_chars += stringLength;

#else
        int stringLength = sprintf_P(timeString, STR_FMT_U, (unsigned int)time);
		printAt(20-stringLength, 3, timeString);
        tft.print(timeString);
#endif
    }

    // Because of the way we're updating the display, we need to clear out everything to the right of the status
    // string
    for (int i = printed_chars; i < 33; ++i) {
        tft.print(" ");
    }

}


#ifdef ESP8266_WiFi
void LcdDisplay::printWiFi(void){
    toggleBacklight = false;  // Assuming we need this

    clear();

    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(WIFI_FONT_SIZE);

    tft.setCursor(0, 0);

    tft.println("mDNS Name: ");
    tft.print(eepromManager.fetchmDNSName());
    tft.println(".local");

    tft.println(" ");

    tft.println("IP Address: ");

    tft.println(WiFi.localIP());

//    lcd.updateBacklight();
}

void LcdDisplay::printWiFiStartup(void){
    toggleBacklight = false;  // Assuming we need this

    clear();

    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(WIFI_FONT_SIZE);

    tft.setCursor(0, 0);

    tft.println("Using a phone, laptop, or ");
    tft.println("other device, connect to ");
    tft.println("the following WiFi network");
    tft.println("to configure this BrewPi");
    tft.println("controller:");
    tft.println("");

    tft.print("AP Name: ");
    tft.println(WIFI_SETUP_AP_NAME);

    tft.print("AP Pass: ");
    tft.println(WIFI_SETUP_AP_PASS);


//    tft.updateBacklight();
}


#endif


void LcdDisplay::printEEPROMStartup(void){

    toggleBacklight = false;  // Assuming we need this

    clear();

    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(WIFI_FONT_SIZE);

    tft.setCursor(0, 0);

    tft.println("Setting up EEPROM...");
    tft.println("Please wait. This");
    tft.println("can take 5+ minutes");
    tft.println("for new installs.");
    tft.println("");

    tft.print("AP Name: ");
    tft.println(WIFI_SETUP_AP_NAME);

    tft.print("AP Pass: ");
    tft.println(WIFI_SETUP_AP_PASS);

}

void LcdDisplay::clear(void) {
    tft.fillScreen(ILI9341_BLACK);
}

void LcdDisplay::clearForText(uint8_t start_x, uint8_t start_y, uint16_t color, uint8_t font_size, uint8_t characters) {
//    uint8_t width = (font_size * characters * 5) + (font_size * (characters-1) * 4);
//    uint8_t height = font_size * 7;

//    tft.fillRect(start_x, start_y, width, height, color);
}



#define FAKE_DISPLAY_COLS 20
#define FAKE_DISPLAY_ROWS 4

void LcdDisplay::getLine(uint8_t lineNumber, char * buffer) {

    std::string line;

    // Could this be replaced with a switch block? Absolutely. Will I? Nope!
    if(lineNumber == 0) {
        line = "Mode  ";


    } else if(lineNumber == 1) {
        line = "Beer  ";


    } else if(lineNumber == 2) {
        line = "Fridge ";



    } else if(lineNumber == 3) {
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
            time = MIN_COOL_ON_TIME-sinceIdleTime;
        }

        else if(state==HEATING_MIN_TIME){
            line += " time left ";
            time = MIN_HEAT_ON_TIME-sinceIdleTime;
        }
        else if(state == WAITING_TO_COOL || state == WAITING_TO_HEAT){
            line += " ";
            time = tempControl.getWaitTime();
        }
        if(time != UINT_MAX){
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

    } else {
        line = "Invalid line requested.";

    }

    const char* src = line.c_str();;
    for (uint8_t i = 0; i < FAKE_DISPLAY_COLS; i++) {
        char c = ' ';
        if(i < line.length())
            c = src[i];
        buffer[i] = (c == 0b11011111) ? 0xB0 : c;
    }
    buffer[FAKE_DISPLAY_COLS] = '\0'; // NULL terminate string


}
