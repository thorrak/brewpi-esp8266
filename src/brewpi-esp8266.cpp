// Libraries have to be loaded in the main .ino file per Visual Micro. Load them here.

#if defined(ESP8266) || defined(ESP32)
#include <FS.h>  // Apparently this needs to be first
#endif

#include "Brewpi.h"

#include <OneWire.h>

#include <Wire.h>

#include "Ticks.h"
#include "Display.h"
#include "TempControl.h"
#include "PiLink.h"
#include "Menu.h"
#include "Pins.h"
#include "RotaryEncoder.h"
#include "Buzzer.h"
#include "TempSensor.h"
#include "TempSensorMock.h"
#include "TempSensorExternal.h"
#include "Ticks.h"
#include "Sensor.h"
#include "SettingsManager.h"
#include "EepromFormat.h"
#include "ESP_WiFi.h"

#if BREWPI_SIMULATE
#include "Simulator.h"
#endif

// global class objects static and defined in class cpp and h files

// instantiate and configure the sensors, actuators and controllers we want to use


/* Configure the counter and delay timer. The actual type of these will vary depending upon the environment.
* They are non-virtual to keep code size minimal, so typedefs and preprocessing are used to select the actual compile-time type used. */
TicksImpl ticks = TicksImpl(TICKS_IMPL_CONFIG);
DelayImpl wait = DelayImpl(DELAY_IMPL_CONFIG);

DisplayType realDisplay;
DisplayType DISPLAY_REF display = realDisplay;

ValueActuator alarm_actuator;



void setup()
{
    // Let's get the display going so that we can provide the user a bit of feedback on what's happening
    display.init();
    display.printEEPROMStartup();

    // Before anything else, let's get SPIFFS working. We need to start it up, and then test if the file system was
    // formatted.
#ifdef ESP8266
    // SPIFFS.begin doesn't allow for formatOnFail as a first argument yet on ESP8266 (but it should be coming soon)
    // https://github.com/esp8266/Arduino/issues/4185
    // TODO - Rewrite this when that PR gets merged	SPIFFS.begin();
    SPIFFS.begin();
#elif defined(ESP32)
    SPIFFS.begin(true);
#else
// We no longer support anything but ESP8266 or ESP32 in this codebase
#error "Invalid platform!"
#endif


    if (!SPIFFS.exists("/formatComplete.txt")) {
        if (!SPIFFS.exists("/mdns.txt")) {
            // This prevents installations that are being upgraded from v0.10 to v0.11 from having their mdns settings
            // wiped out
            SPIFFS.format();
        }
        File f = SPIFFS.open("/formatComplete.txt", "w");
        f.close();
    }

    initialize_wifi();

#if BREWPI_BUZZER	
	buzzer.init();
	buzzer.beep(2, 500);
#endif	

	piLink.init();  // Initializes either the serial or telnet connection

	logDebug("started");
	tempControl.init();
	settingsManager.loadSettings();

#if BREWPI_SIMULATE
	simulator.step();
	// initialize the filters with the assigned initial temp value
	tempControl.beerSensor->init();
	tempControl.fridgeSensor->init();
#endif

	// Once the WiFi and piLink are initialized, we want to display a screen with connection information
    display_connect_info_and_create_callback();

	display.clear();
	display.printStationaryText();
	display.printState();

//	rotaryEncoder.init();

	logDebug("init complete");
}



void brewpiLoop(void)
{
	static unsigned long lastUpdate = 0;
    static unsigned long lastLcdUpdate = 0;

	uint8_t oldState;
#ifndef BREWPI_TFT  // We don't want to do this for the TFT display
    if(ticks.millis() - lastLcdUpdate >= (180000)) { //reset lcd every 180 seconds as a workaround for screen scramble
        lastLcdUpdate = ticks.millis();

        display.init();
        display.printStationaryText();
        display.printState();

        rotaryEncoder.init();
    }
#endif

    if (ticks.millis() - lastUpdate >= (1000)) { //update settings every second
		lastUpdate = ticks.millis();

#if BREWPI_BUZZER
		buzzer.setActive(alarm_actuator.isActive() && !buzzer.isActive());
#endif			

		tempControl.updateTemperatures();
		tempControl.detectPeaks();
		tempControl.updatePID();
		oldState = tempControl.getState();
		tempControl.updateState();
		if (oldState != tempControl.getState()) {
			piLink.printTemperatures(); // add a data point at every state transition
		}
		tempControl.updateOutputs();

#if BREWPI_MENU
		if (rotaryEncoder.pushed()) {
			rotaryEncoder.resetPushed();
			menu.pickSettingToChange();
		}
#endif

		// update the lcd for the chamber being displayed
		display.printState();
		display.printAllTemperatures();
		display.printMode();
#ifndef BREWPI_TFT
		display.updateBacklight();
#endif
	}

	//listen for incoming connections while waiting to update
    wifi_connect_clients();
	piLink.receive();

}

void loop() {
#if BREWPI_SIMULATE
	simulateLoop();
#else
	brewpiLoop();
#endif
}

