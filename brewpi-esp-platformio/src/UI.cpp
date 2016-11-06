#include "Brewpi.h"
#include "UI.h"
#include "RotaryEncoder.h"
#include "Display.h"
#include "Buzzer.h"
#include "Menu.h"
#include "ActuatorInterfaces.h"

DisplayType realDisplay;
DisplayType DISPLAY_REF display = realDisplay;


uint8_t UI::init() {
#if BREWPI_BUZZER
    buzzer.init();
	buzzer.beep(2, 500);
#endif
    display.init();
#if BREWPI_ROTARY_ENCODER
    rotaryEncoder.init();
#endif
    return 0;
}

uint32_t UI::showStartupPage()
{
#ifdef ESP8266_WiFi
    display.printWiFi();
    delay(8000);  // 8 second delay for startup
    display.clear();  // Clear after printing WiFi (because apparently this doesn't happen otherwise)
    return 1;
#else
    return 0;
#endif
}

/**
 * Show the main controller page. 
 */
void UI::showControllerPage()
{
    display.printStationaryText();
    display.printState();
}

extern ActuatorBool alarm;
void UI::ticks() {

#if BREWPI_MENU && BREWPI_ROTARY_ENCODER
    if(rotaryEncoder.pushed()){
        rotaryEncoder.resetPushed();
        menu.pickSettingToChange();
    }
#endif

}

void UI::update() {

    // update the lcd for the chamber being displayed
    display.printState();
    display.printAllTemperatures();
    display.printMode();
    display.updateBacklight();

}

bool UI::inStartup() { return false; }