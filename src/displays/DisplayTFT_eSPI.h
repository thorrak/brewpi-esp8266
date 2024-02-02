//
// Created by John Beeler on 01/22/24.
//

#ifndef DISPLAYTFT_ESPI_H
#define DISPLAYTFT_ESPI_H

#ifdef BREWPI_TFT_ESPI

#include "Brewpi.h"
#include "DisplayBase.h"


#define DISABLE_ALL_LIBRARY_WARNINGS
#include <TFT_eSPI.h>
#undef DISABLE_ALL_LIBRARY_WARNINGS

#define FF17                    &FreeMono9pt7b
#define GFXFF                   1  // THis can probably be removed


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
    // printIPAddressInfo();
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
    // void printIPAddressInfo();

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

    // print a temperature
    void printTemperature(temperature temp, uint8_t start_x, uint8_t start_y);
    void printTemperatureAtMonoChars(uint8_t x_chars, uint8_t y_chars, temperature temp);
    void printAtMonoChars(uint8_t x_chars, uint8_t y_chars, const char *text);

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

private:

    uint8_t stateOnDisplay;
    uint8_t flags;

    uint8_t printTime(uint16_t time);
};

extern TFT_eSPI tft;


#endif  //BREWPI_TFT

#endif //DISPLAYTFT_ESPI_H
