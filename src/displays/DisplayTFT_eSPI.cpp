/*
 * Copyright 2024 John Beeler.
 *
 * This file has been added to BrewPi by the author noted above. It should be assumed to be available under the same
 * license terms as BrewPi, unless separate arrangements have been made with the author.
 *
 */

#ifdef BREWPI_TFT_ESPI

#include "Brewpi.h"
#include "BrewpiStrings.h"
#include <limits.h>
#include <stdint.h>
#include <string>

#include "Display.h"

#include "DisplayTFT_eSPI.h"

#include "TempControl.h"
#include "TemperatureFormats.h"

#if !defined(ESP32)
#error "Invalid chipset!"
#endif

// ESP32 Only
#include <WiFi.h> // For printing the IP address


#if defined(HAS_AXP192)
#include "AXP192.h"
AXP192 Axp = AXP192();
#endif


TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);
bool toggleBacklight = false;  // Not used for this screen type


#ifndef min
#define min _min
#endif

#ifndef max
#define max _max
#endif


LcdDisplay::LcdDisplay() {
    // Initialize the display device -- we can reinitialize later if needed
    // tft = new TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);

    // We can't do much more as this object gets created at boot
}

LcdDisplay::~LcdDisplay() {
    // delete tft;
}


void LcdDisplay::print_layout() {

    // For now, we're using the basic "four line" layout for this device. Disable anything else

    // Print the lines for the basic layout
//     tft.drawLine(0, BEER_NAME_LINE_Y, 320, BEER_NAME_LINE_Y, ILI9341_WHITE);  // Bar beneath beer name/set points
//     tft.drawLine(0, ACTUAL_SET_LINE_Y, 320, ACTUAL_SET_LINE_Y, ILI9341_DARKGREY);  // Bar between set/measured points
//     tft.drawLine(0, MIDDLE_BAR_Y_END, 320, MIDDLE_BAR_Y_END, ILI9341_WHITE);  // Bar beneath set/measured points
//     tft.drawLine(MIDDLE_BAR_X, BEER_NAME_LINE_Y, MIDDLE_BAR_X, MIDDLE_BAR_Y_END, ILI9341_WHITE);  // Middle bar

//     tft.drawLine(GRAVITY_LINE_X, GRAVITY_LINE_START_Y, GRAVITY_LINE_X, 240, ILI9341_WHITE);  // Gravity bar


//     // Print the headers
//     tft.setTextSize(HEADER_FONT_SIZE);
//     tft.setCursor(FRIDGE_HEADER_START_X, HEADER_START_Y);
    printAtMonoChars(0, 2, (flags & LCD_FLAG_DISPLAY_ROOM) ?  "Room  " : "Fridge");


//     tft.setTextSize(HEADER_FONT_SIZE);
//     tft.setCursor(BEER_HEADER_START_X, HEADER_START_Y);
    printAtMonoChars(0, 1, "Beer");

//     tft.setTextSize(SET_HEADER_FONT_SIZE);
//     tft.setCursor(FRIDGE_SET_HEADER_START_X, SET_HEADER_START_Y);
//     tft.print("Set");
//     tft.setTextSize(SET_HEADER_FONT_SIZE);
//     tft.setCursor(BEER_SET_HEADER_START_X, SET_HEADER_START_Y);
//     tft.print("Set");

//     // Print the "Now Fermenting" mesage
//     tft.setTextSize(BEER_NAME_FONT_SIZE);
//     tft.setCursor(BEER_NAME_START_X, BEER_NAME_START_Y);
// //    tft.print("Current Beer: Belgian Beer Series - The Saisoning");

}

void LcdDisplay::init() {

#ifdef HAS_AXP192
    // For m5 stick and whatnot, the LCD backlight AND the controller both are powered off the AXP192, so we need to initialize that first
    Axp.begin();
    delay(50);    
#endif

    tft.init();
    // reinit();

    stateOnDisplay = 0xFF; // set to unknown state to force update
    flags = LCD_FLAG_ALTERNATE_ROOM;  // TODO - Test with a room sensor to see what happens

    tft.begin();
    if (extendedSettings.invertTFT)
        tft.setRotation(3);
    else
        tft.setRotation(1);

    tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(FF17);


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
	printTemperatureAtMonoChars(12, 1, tempControl.getBeerTemp());
}

void LcdDisplay::printBeerSet(){
//    temperature beerSet = tempControl.getBeerSetting();
    printTemperatureAtMonoChars(12, 1, tempControl.getBeerSetting());

}

void LcdDisplay::printFridgeTemp(){
    printTemperatureAtMonoChars(6,2, flags & LCD_FLAG_DISPLAY_ROOM ?
		tempControl.ambientSensor->read() :
		tempControl.getFridgeTemp());
}

void LcdDisplay::printFridgeSet(){
    temperature fridgeSet = tempControl.getFridgeSetting();
    if(flags & LCD_FLAG_DISPLAY_ROOM) // beer setting is not active
        fridgeSet = INVALID_TEMP;
	printTemperatureAtMonoChars(12, 2, fridgeSet);
}

void LcdDisplay::printTemperatureAtMonoChars(uint8_t x_chars, uint8_t y_chars, temperature temp){
    uint16_t x = x_chars * tft.textWidth("A", GFXFF) + 3;
    uint16_t y = (y_chars) * tft.fontHeight(GFXFF) + 2;

    printTemperature(temp, x, y);
}

void LcdDisplay::printAtMonoChars(uint8_t x_chars, uint8_t y_chars, const char *text){
    uint16_t x = x_chars * tft.textWidth("A", GFXFF) + 3;
    uint16_t y = (y_chars) * tft.fontHeight(GFXFF) + 2;

    tft.drawString(text, x, y);
}


void LcdDisplay::printTemperature(temperature temp, uint8_t start_x, uint8_t start_y){
    if (temp==INVALID_TEMP) {
        tft.drawString("  --.-", start_x, start_y);
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

    tft.drawString(tempBuf, start_x, start_y);
}

//print the stationary text on the lcd.
void LcdDisplay::printStationaryText(){
    print_layout();
}

void LcdDisplay::printMode(){
    printAtMonoChars(0, 0, "Mode   ");

    switch(tempControl.getMode()){
        case Modes::fridgeConstant:
            printAtMonoChars(8, 0, "Fridge Const");
            break;
        case Modes::beerConstant:
            printAtMonoChars(8, 0, "Beer Const  ");
            break;
        case Modes::beerProfile:
            printAtMonoChars(8, 0, "Beer Profile");
            break;
        case Modes::off:
            printAtMonoChars(8, 0, "Off         ");
            break;
        case Modes::test:
            printAtMonoChars(8, 0, "** Testing *");
            break;
        default:
            printAtMonoChars(8, 0, "Invalid Mode");
            break;
    }
}

// void LcdDisplay::printIPAddressInfo(){
//     tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
//     tft.setTextSize(IP_ADDRESS_FONT_SIZE);
//     clearForText(IP_ADDRESS_START_X, IP_ADDRESS_START_Y, ILI9341_BLACK, MODE_FONT_SIZE, 21);

//     tft.setCursor(IP_ADDRESS_START_X, IP_ADDRESS_START_Y);
//     tft.print("IP Address: ");

//     if(WiFi.isConnected()) {
//         tft.print(WiFi.localIP());
//     } else {
//         tft.print("Disconnected");
//     }

// }


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
    // tft.print(printString);
    printAtMonoChars(12, 3, printString);

    return stringLength;

#else
#warning "This has not been tested"
    int stringLength = sprintf_P(timeString, STR_FMT_U, (unsigned int)time);
    printAt(20-stringLength, 3, timeString);
    tft.print(timeString);
    return stringLength;
#endif
}

// print the current state on the last line of the lcd
void LcdDisplay::printState(){
    uint16_t time = UINT16_MAX; // init to max
    uint8_t state = tempControl.getDisplayState();
    uint8_t printed_chars = 0;


    // For the TFT, always reprint the state text
    switch (state){
        case IDLE:
            printAtMonoChars(0, 3, "Idling for ");
            printed_chars += 11;
            break;
        case WAITING_TO_COOL:
            printAtMonoChars(0, 3, "Wait to cool ");
            printed_chars += 13;
            break;
        case WAITING_TO_HEAT:
            printAtMonoChars(0, 3, "Wait to heat ");
            printed_chars += 13;
            break;
        case WAITING_FOR_PEAK_DETECT:
            printAtMonoChars(0, 3, "Waiting for peak    ");
            printed_chars += 16;
            break;
        case COOLING:
            printAtMonoChars(0, 3, "Cooling for ");
            printed_chars += 12;
            break;
        case HEATING:
            printAtMonoChars(0, 3, "Heating for ");
            printed_chars += 12;
            break;
        case COOLING_MIN_TIME:
            printAtMonoChars(0, 3, "Cool time left ");
            printed_chars += 15;
            break;
        case HEATING_MIN_TIME:
            printAtMonoChars(0, 3, "Heat time left ");
            printed_chars += 15;
            break;
        case DOOR_OPEN:
            printAtMonoChars(0, 3, "Door open ");
            printed_chars += 9;
            break;
        case STATE_OFF:
            printAtMonoChars(0, 3, "Off                 ");
            printed_chars += 20;
            break;
        default:
            printAtMonoChars(0, 3, "Unknown status!     ");
            printed_chars += 20;
            break;
    }

    uint16_t sinceIdleTime = tempControl.timeSinceIdle();
    if(state==IDLE){
        time = 	min(tempControl.timeSinceCooling(), tempControl.timeSinceHeating());
        printed_chars += printTime(time);
    } else if(state==COOLING || state==HEATING){
        time = sinceIdleTime;
        printed_chars += printTime(time);
    } else if(state==COOLING_MIN_TIME){
        time = tempControl.getMinCoolOnTime()-sinceIdleTime;
        printed_chars += printTime(time);
    } else if(state==HEATING_MIN_TIME){
        time = tempControl.getMinHeatOnTime()-sinceIdleTime;
        printed_chars += printTime(time);
    } else if(state == WAITING_TO_COOL || state == WAITING_TO_HEAT){
        time = tempControl.getWaitTime();
        printed_chars += printTime(time);
    }

    // Because of the way we're updating the display, we need to clear out everything to the right of the status
    // string
    for (uint8_t i = printed_chars; i < 20; ++i) {
        // printAtMonoChars(i+1, 3, ".");
    }
}


#ifdef ESP8266_WiFi
void LcdDisplay::printWiFi(){
    clear();

    printAtMonoChars(0, 0, "mDNS Name: ");
    printAtMonoChars(0, 1, eepromManager.fetchmDNSName().c_str());
    printAtMonoChars(strlen(eepromManager.fetchmDNSName().c_str()), 1, ".local");

    printAtMonoChars(0, 3, "IP Address: ");
    printAtMonoChars(0, 4, WiFi.localIP().toString().c_str());
}

void LcdDisplay::printWiFiStartup(){
    clear();

	printAtMonoChars(0, 0, "Connect to this AP:");
    printAtMonoChars(0, 1, "AP Name: ");
    printAtMonoChars(10, 1, WIFI_SETUP_AP_NAME);
    printAtMonoChars(0, 2, "AP Pass: ");
    printAtMonoChars(10, 2, WIFI_SETUP_AP_PASS);
    printAtMonoChars(0, 3, "to configure device");

}

void LcdDisplay::printWiFiConnect(){
    clear();

    printAtMonoChars(0, 0, "Connecting to WiFi");
    printAtMonoChars(0, 1, "Please wait up to");
    printAtMonoChars(0, 2, "60 seconds.");

}

#endif

#ifdef HAS_BLUETOOTH
void LcdDisplay::printBluetoothStartup(){
    clear();

    printAtMonoChars(0, 0, "Performing initial");
    printAtMonoChars(0, 1, "bluetooth scan");
    printAtMonoChars(0, 3, "Please wait...");
}

void LcdDisplay::printGravity(){
    // tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    // tft.setTextSize(GRAVITY_HEADER_FONT_SIZE);
    // clearForText(GRAVITY_START_X, GRAVITY_HEADER_START_Y, ILI9341_BLACK, GRAVITY_HEADER_FONT_SIZE, 7);
    // clearForText(GRAVITY_START_X, GRAVITY_START_Y, ILI9341_BLACK, GRAVITY_HEADER_FONT_SIZE, 5);

    // tft.setCursor(GRAVITY_START_X, GRAVITY_HEADER_START_Y);
    // tft.print("Gravity");


    // // Print the Gravity
    // double grav = 80.0 / 1000.0;



    // char grav_text[6];
    // snprintf(grav_text, 6, "%05.3f", grav);
    // tft.setTextSize(GRAVITY_FONT_SIZE);
    // tft.setCursor(GRAVITY_START_X, GRAVITY_START_Y);
    // tft.print(grav_text);
}
#endif


void LcdDisplay::clear() {
    tft.fillScreen(TFT_BLACK);
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