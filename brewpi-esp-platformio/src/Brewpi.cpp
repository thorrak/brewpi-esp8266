
#ifdef ESP8266
#include <FS.h>  // Apparently this needs to be first
#endif

#include <Arduino.h>
#undef min
#undef max


//#include <stdio.h>
//#include <stdlib.h>
//#include <string>

#include "Platform.h"
#include "Brewpi.h"

#ifdef ESP8266_WiFi_Control
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WiFi.h>		//ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include "ESP8266mDNS.h"
#include <ESP8266WebServer.h>	//Local WebServer used to serve the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <EEPROM.h>				//For storing the configuration constants
#endif

#include <OneWire.h>

#include <Wire.h>

#include "Ticks.h"
#include "Display.h"
#include "TempControl.h"
#include "PiLink.h"
#include "TempSensorBasic.h"
#include "TempSensorMock.h"
#include "TempSensorExternal.h"
#include "ActuatorMocks.h"
#include "Ticks.h"
#include "Sensor.h"
#include "SettingsManager.h"

#include "UI.h"


#if BREWPI_SIMULATE
#include "Simulator.h"
#endif

// global class objects static and defined in class cpp and h files

// instantiate and configure the sensors, actuators and controllers we want to use


void setup(void);
void loop(void);

/* Configure the counter and delay timer. The actual type of these will vary depending upon the environment.
* They are non-virtual to keep code size minimal, so typedefs and preprocessing are used to select the actual compile-time type used. */
TicksImpl ticks = TicksImpl(TICKS_IMPL_CONFIG);
DelayImpl wait = DelayImpl(DELAY_IMPL_CONFIG);

UI ui;


void setup()
{
    bool resetEeprom = platform_init();

    eepromManager.init();
    ui.init();
    piLink.init();

#ifdef ESP8266_WiFi
    // If we're using WiFi, initialize the bridge
	server.begin();
	server.setNoDelay(true);
#endif

    logDebug("started");

    uint32_t start = ticks.millis();
    uint32_t delay = ui.showStartupPage();
    while (ticks.millis() - start <= delay) {
        ui.ticks();
    }

    // initialize OneWire
#ifndef ARDUINO // This is noop on esp8266/arduino
    if (!primaryOneWireBus.init()) {
		logError(ERROR_ONEWIRE_INIT_FAILED);
	}
#endif

#if BREWPI_SIMULATE
    simulator.step();
	// initialize the filters with the assigned initial temp value
	//tempControl.beerSensor->init();
	//tempControl.fridgeSensor->init();
#endif
    settingsManager.loadSettings();

    control.update(); // TODO - Determine if I need this here if we have no menu/rotary encoder

    ui.showControllerPage();


    // flush any waiting input.
    // Linux can put garbage in the serial input buffer during connect
    piLink.flushInput();

    logDebug("init complete");
}



void brewpiLoop(void)
{
    // TODO - Determine if this makes sense...
    static unsigned long lastUpdate = 0; // init at -1000 to update immediately
    ui.ticks();
    if(!ui.inStartup() && (ticks.millis() - lastUpdate >= (1000))) { //update settings every second
        lastUpdate = ticks.millis();
        control.update();
        ui.update();
    }

    control.fastUpdate(); // update actuators as often as possible for PWM

#ifdef ESP8266_WiFi
    yield();
	connectClients();
	yield();
#endif

    piLink.receive();  //listen for incoming serial connections while waiting to update
}



void loop() {
#if BREWPI_SIMULATE
    simulateLoop();
#else
    brewpiLoop();
#endif
}

