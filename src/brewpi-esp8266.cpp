
#ifdef ESP8266
#include <LittleFS.h>  // Apparently this needs to be first
#elif defined(ESP32)
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
#include "ESP_BP_WiFi.h"
#include "CommandProcessor.h"
#include "PromServer.h"
#include "wireless/BTScanner.h"

#if BREWPI_SIMULATE
#include "Simulator.h"
#endif

/**
 * \file brewpi-esp8266.cpp
 *
 * \brief Main project entrypoint
 */

// global class objects static and defined in class cpp and h files
// instantiate and configure the sensors, actuators and controllers we want to use


/*
 * Create the correct type of PiLink connection for how we're configured.  When
 * everything gets moved off of the deprecated "build JSON via string fragment
 * prints" interface, CompatiblePiLink can be replaced with PiLink.
 */
#if defined(ESP8266_WiFi)
// Just use the serverClient object as it supports all the same functions as Serial
extern WiFiClient serverClient;
CompatiblePiLink<WiFiClient> piLink(serverClient);
#else
// Not using ESP8266 WiFi
CompatiblePiLink<HardwareSerial> piLink(Serial);
#endif

/* Configure the counter and delay timer. The actual type of these will vary depending upon the environment.
* They are non-virtual to keep code size minimal, so typedefs and preprocessing are used to select the actual compile-time type used. */
TicksImpl ticks = TicksImpl(TICKS_IMPL_CONFIG);
DelayImpl wait = DelayImpl(DELAY_IMPL_CONFIG);

DisplayType realDisplay;
DisplayType DISPLAY_REF display = realDisplay;

ValueActuator alarm_actuator;

/**
 * \brief Restart the board
 */
void handleReset()
{
    // The asm volatile method doesn't work on ESP8266. Instead, use ESP.restart
    ESP.restart();
}

/**
 * \brief Startup configuration
 *
 * - Start up the filesystem
 * - Initialize the display
 * - If in simulation mode, bootstrap the simulator
 * - Start the PiLink connection (either tcp socket or serial, depending on compile time configuration)
 */
void setup()
{
    // Let's get the display going so that we can provide the user a bit of feedback on what's happening
    display.init();
    display.printEEPROMStartup();

    // Before anything else, let's get the filesystem working. We need to start it up, and then test if the file system
    // wasformatted.
    FILESYSTEM.begin(true);

    initialize_wifi();

#if BREWPI_BUZZER
	buzzer.init();
	buzzer.beep(2, 500);
#endif


	piLink.init();  // Initializes either the serial or telnet connection

#ifdef HAS_BLUETOOTH
    bt_scanner.init();
    bt_scanner.scan();
    DisplayType::printBluetoothStartup();  // Alert the user as to the startup delay
    delay(10000);
#endif

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

  if(Config::Prometheus::enable())
    promServer.setup();

//	rotaryEncoder.init();

	logDebug("init complete");
}



/**
 * \brief Main execution loop
 */
void brewpiLoop()
{
	static unsigned long lastUpdate = 0;
	uint8_t oldState;
#ifndef BREWPI_TFT  // We don't want to do this for the TFT display
    static unsigned long lastLcdUpdate = 0;
    if(ticks.millis() - lastLcdUpdate >= (180000)) { //reset lcd every 180 seconds as a workaround for screen scramble
        lastLcdUpdate = ticks.millis();

        DisplayType::init();
        DisplayType::printStationaryText();
        DisplayType::printState();

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
          piLink.sendStateNotification();
		}
		tempControl.updateOutputs();

#if BREWPI_MENU
		if (rotaryEncoder.pushed()) {
			rotaryEncoder.resetPushed();
			menu.pickSettingToChange();
		}
#endif

		// update the lcd for the chamber being displayed
        DisplayType::printState();
        DisplayType::printAllTemperatures();
        DisplayType::printMode();
#ifndef BREWPI_TFT
        DisplayType::updateBacklight();
#endif
	}

	//listen for incoming connections while waiting to update
  wifi_connect_clients();
  CommandProcessor::receiveCommand();

#ifdef HAS_BLUETOOTH
  bt_scanner.scan();        // Check/restart scan 
#endif
}


/**
 * \brief Main execution loop
 *
 * This dispatches to brewpiLoop(), or if we're in simulation mode simulateLoop()
 */
void loop() {
#if BREWPI_SIMULATE
	simulateLoop();
#else
	brewpiLoop();
#endif
}

