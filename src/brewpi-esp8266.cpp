
#ifdef ESP8266
#include <FS.h>  // Apparently this needs to be first
#include <LittleFS.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <FS.h>  // Apparently this needs to be first
#endif

#include "Brewpi.h"

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
#include "ESP_BP_WiFi.h"
#include "CommandProcessor.h"
#include "PromServer.h"
#include "wireless/BTScanner.h"
#include "tplink/TPLinkScanner.h"

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
 * Create the correct type of PiLink connection for how we're configured.
 */
#if defined(ESP8266_WiFi)
// Just use the serverClient object as it supports all the same functions as Serial
// extern WiFiClient serverClient;
PiLink<WiFiClient> piLink(serverClient);
#else
// Not using ESP8266 WiFi
#if defined(ESP32S2)
// The ESP32-S2 has USB built onto the chip, and uses USBCDC rather than HardwareSerial
PiLink<USBCDC> piLink(Serial);
#else
PiLink<HardwareSerial> piLink(Serial);
#endif
#endif

/* Configure the counter and delay timer. The actual type of these will vary depending upon the environment.
* They are non-virtual to keep code size minimal, so typedefs and preprocessing are used to select the actual compile-time type used. */
TicksImpl ticks = TicksImpl(TICKS_IMPL_CONFIG);
DelayImpl wait = DelayImpl(DELAY_IMPL_CONFIG);

DisplayType realDisplay;
DisplayType DISPLAY_REF display = realDisplay;

ValueActuator alarm_actuator;

#ifdef ESP32
void printMem() {
    char buf[256];
    const uint32_t free = ESP.getFreeHeap();
    const uint32_t max = ESP.getMaxAllocHeap();
    const uint8_t frag = 100 - (max * 100) / free;
    sprintf(buf, "Free Heap: %d, Largest contiguous block: %d, Frag: %d%%\r\n", free, max, frag );
    // PiLink.print(F(), free, max, frag);
    piLink.print(buf);
}
#endif

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
#ifdef ESP8266_WiFi
    Serial.begin(Config::PiLink::serialSpeed);
#endif


    // Before anything else, let's get the filesystem working. We need to start it up, and then test if the file system
    // was formatted.
  #ifdef ESP32
    FILESYSTEM.begin(true);
  #else
    FILESYSTEM.begin();
  #endif

  extendedSettings.loadFromSpiffs();
  display.init();

    initialize_wifi();

#if BREWPI_BUZZER
	buzzer.init();
	buzzer.beep(2, 500);
#endif


	piLink.init();  // Initializes either the serial or telnet connection

#ifdef EXTERN_SENSOR_ACTUATOR_SUPPORT
  // Initialize UDP and send the initial discovery message
  // TODO - Test how this reacts when WiFi is not available
  tp_link_scanner.init();
  tp_link_scanner.send_discover();
  delay(200); // This should be very quick
  tp_link_scanner.process_udp_incoming();
#endif

#ifdef HAS_BLUETOOTH
    bt_scanner.init();
    bt_scanner.scan();
    DisplayType::printBluetoothStartup();  // Alert the user about the startup delay
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
    // printMem();

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
    DisplayType::printAll();
	}

	//listen for incoming connections while waiting to update
  wifi_connect_clients();
  CommandProcessor::receiveCommand();

#ifdef HAS_BLUETOOTH
  bt_scanner.scan();        // Check/restart scan 
#endif

#ifdef EXTERN_SENSOR_ACTUATOR_SUPPORT
  tp_link_scanner.scan_and_refresh();
#endif

#ifdef ESP8266
  MDNS.update();
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

