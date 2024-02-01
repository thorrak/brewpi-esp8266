//
// Created by John Beeler on 11/28/19.
//

#ifndef LEGACY_PLATFORMIO_DISPLAYTFT_H
#define LEGACY_PLATFORMIO_DISPLAYTFT_H

#ifdef BREWPI_TFT_ILI9341

#include "Brewpi.h"
#include "DisplayBase.h"

#include "Adafruit_ILI9341.h"


/************** Alignment Markers ****************/

// Beer Name Header
#define BEER_NAME_FONT_SIZE 1
#define BEER_NAME_START_X 2 // Bring it off the left side of the screen
#define BEER_NAME_START_Y 2 // Bring it off the top of the screen

// Beer Name Line
#define BEER_NAME_LINE_Y (BEER_NAME_START_Y + (BEER_NAME_FONT_SIZE*7 + 7))

// Generic Lines for Layout
#define MIDDLE_BAR_X (320/2)
#define MIDDLE_BAR_Y_END (240/4*3)


// Fridge/Beer Headers
#define HEADER_FONT_SIZE 2
#define HEADER_START_Y (BEER_NAME_LINE_Y + 7)  // Below the line at the top of the screen
#define FRIDGE_HEADER_START_X (MIDDLE_BAR_X/2 - (5*5*HEADER_FONT_SIZE + 4*HEADER_FONT_SIZE)/2)
#define BEER_HEADER_START_X (MIDDLE_BAR_X + MIDDLE_BAR_X/2) - (4*5*HEADER_FONT_SIZE + 3*HEADER_FONT_SIZE)/2


// Beer/Fridge Temp Display
#define MAIN_TEMP_FONT_SIZE 3
#define MAIN_TEMP_START_Y (HEADER_START_Y + (HEADER_FONT_SIZE * 7 + 14))
#define FRIDGE_TEMP_START_X (MIDDLE_BAR_X/2 - (8*5*MAIN_TEMP_FONT_SIZE + 8*MAIN_TEMP_FONT_SIZE)/2)
#define FRIDGE_TEMP_END_X (FRIDGE_TEMP_START_X + (8*5*MAIN_TEMP_FONT_SIZE + 7*MAIN_TEMP_FONT_SIZE))
#define BEER_TEMP_START_X (MIDDLE_BAR_X + MIDDLE_BAR_X/2) - (8*5*MAIN_TEMP_FONT_SIZE + 4*MAIN_TEMP_FONT_SIZE)/2
#define BEER_TEMP_END_X (BEER_TEMP_START_X + (8*5*MAIN_TEMP_FONT_SIZE + 4*MAIN_TEMP_FONT_SIZE))

// Fridge/Beer Actual vs. Set Line
#define ACTUAL_SET_LINE_Y (MAIN_TEMP_START_Y + (MAIN_TEMP_FONT_SIZE * 7 + 11))


// Fridge/Beer "Set" Header
#define SET_HEADER_FONT_SIZE HEADER_FONT_SIZE
#define SET_HEADER_START_Y (ACTUAL_SET_LINE_Y + 7)  // Place it just below the line separating the two
#define FRIDGE_SET_HEADER_START_X (MIDDLE_BAR_X/2 - (3*5*SET_HEADER_FONT_SIZE + 2*SET_HEADER_FONT_SIZE)/2)
#define BEER_SET_HEADER_START_X (MIDDLE_BAR_X + MIDDLE_BAR_X/2) - (3*5*SET_HEADER_FONT_SIZE + 2*SET_HEADER_FONT_SIZE)/2


// Beer/Fridge Set Temp Display
#define SET_TEMP_FONT_SIZE 2
#define SET_TEMP_START_Y SET_HEADER_START_Y + (SET_HEADER_FONT_SIZE * 7 + 14)
#define FRIDGE_SET_START_X (MIDDLE_BAR_X/2 - (8*5*SET_TEMP_FONT_SIZE + 7*SET_TEMP_FONT_SIZE)/2)
#define FRIDGE_SET_END_X (FRIDGE_SET_START_X + (8*5*SET_TEMP_FONT_SIZE + 7*SET_TEMP_FONT_SIZE))
#define BEER_SET_START_X (MIDDLE_BAR_X + MIDDLE_BAR_X/2) - (8*5*SET_TEMP_FONT_SIZE + 7*SET_TEMP_FONT_SIZE)/2
#define BEER_SET_END_X (BEER_SET_START_X + (8*5*SET_TEMP_FONT_SIZE + 7*SET_TEMP_FONT_SIZE))

// Mode/Status Display Lines
#define MODE_FONT_SIZE 1
#define MODE_START_X 2
#define MODE_START_Y (MIDDLE_BAR_Y_END + 7)

#define STATUS_FONT_SIZE 1
#define STATUS_START_X 2
#define STATUS_START_Y (MODE_START_Y + (MODE_FONT_SIZE * 7 + 7))

#define IP_ADDRESS_FONT_SIZE 1
#define IP_ADDRESS_START_X 2
#define IP_ADDRESS_START_Y (STATUS_START_Y + (MODE_FONT_SIZE * 7 + 7))

// Gravity Display
#define GRAVITY_FONT_SIZE 3
#define GRAVITY_HEADER_FONT_SIZE 2
#define GRAVITY_START_X (320-7-(5*GRAVITY_FONT_SIZE*5+4*GRAVITY_FONT_SIZE))
#define GRAVITY_HEADER_START_Y MODE_START_Y

#define GRAVITY_START_Y (GRAVITY_HEADER_START_Y + (GRAVITY_HEADER_FONT_SIZE*7+14))

#define GRAVITY_LINE_X (GRAVITY_START_X - 7)
#define GRAVITY_LINE_START_Y MIDDLE_BAR_Y_END


// WiFi Screen
#define WIFI_FONT_SIZE 2



class LcdDisplay
{
public:
    LcdDisplay();
    ~LcdDisplay();
    // initializes the lcd display
    void init();

    void printAll()
    {
    //     printStationaryText();
    printState();
    printAllTemperatures();
    printMode();
    printIPAddressInfo();
#ifdef HAS_BLUETOOTH
    // printGravity();
#endif
    }

    // print all temperatures on the LCD
    void printAllTemperatures();

    // print the stationary text on the lcd.
    void printStationaryText();
    void print_layout();  // TFT-only replacement for printStationaryText

    // print mode on the right location on the first line, after Mode:
    void printMode();
    void printIPAddressInfo();

    void setDisplayFlags(uint8_t newFlags);
    uint8_t getDisplayFlags() { return flags; };

    // print beer temperature at the right place on the display
    void printBeerTemp();

    // print beer temperature setting at the right place on the display
    void printBeerSet();

    // print fridge temperature at the right place on the display
    void printFridgeTemp();

    // print fridge temperature setting at the right place on the display
    void printFridgeSet();

    // print the current state on the last line of the LCD
    void printState();

    void getLine(uint8_t lineNumber, char * buffer);
//
//    void setBufferOnly(bool bufferOnly)
//    {
//    lcd.setBufferOnly(bufferOnly);
//    }

//    void resetBacklightTimer() { lcd.resetBacklightTimer(); }
//    void updateBacklight() { lcd.updateBacklight(); }

    // print a temperature
    void printTemperature(temperature temp, uint8_t font_size);
    void printTemperatureAt(uint8_t x, uint8_t y, uint8_t font_size, temperature temp);

#ifdef ESP8266_WiFi
    void printWiFi();
    void printWiFiStartup();
    void printWiFiConnect();
#endif

#ifdef HAS_BLUETOOTH
    void printBluetoothStartup();
    void printGravity();
#endif

    void clear();
    void clearForText(uint8_t start_x, uint8_t start_y, uint16_t color, uint8_t font_size, uint8_t characters);


private:
    Adafruit_ILI9341 *tft;

    uint8_t stateOnDisplay;
    uint8_t flags;

    uint8_t printTime(uint16_t time);
};



#endif  // BREWPI_TFT_ILI9341

#endif // LEGACY_PLATFORMIO_DISPLAYTFT_H
