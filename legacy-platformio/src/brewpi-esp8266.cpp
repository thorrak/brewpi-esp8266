// Libraries have to be loaded in the main .ino file per Visual Micro. Load them here.

#ifdef ESP8266
#include <FS.h>  // Apparently this needs to be first
#endif

#include "Brewpi.h"

#ifdef ESP8266_WiFi_Control
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>		//ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>	//Local WebServer used to serve the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "Version.h" 			// Used in mDNS announce string
#endif

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

ValueActuator alarm;

#ifdef ESP8266_WiFi
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}

// Not sure if this is sufficient to test for validity
bool isValidmDNSName(String mdns_name) {
	for (std::string::size_type i = 0; i < mdns_name.length(); ++i) {
		// For now, we're just checking that every character in the string is alphanumeric. May need to add more validation here.
		if (!isalnum(mdns_name[i]))
			return false;
	}
	return true;
}

WiFiServer server(23);
WiFiClient serverClient;
#endif

void handleReset()
{
#if defined(ESP8266)
	// The asm volatile method doesn't work on ESP8266. Instead, use ESP.restart
	ESP.restart();
#else
	// resetting using the watchdog timer (which is a full reset of all registers) 
	// might not be compatible with old Arduino bootloaders. jumping to 0 is safer.
	asm volatile ("  jmp 0");
#endif
}



void setup()
{


#ifdef ESP8266_WiFi
	String mdns_id;

	mdns_id = eepromManager.fetchmDNSName();
	if(mdns_id.length()<=0)
		mdns_id = "ESP" + String(ESP.getChipId());


	// If we're going to set up WiFi, let's get to it
	WiFiManager wifiManager;
	wifiManager.setConfigPortalTimeout(5*60); // Time out after 5 minutes so that we can keep managing temps 
	wifiManager.setDebugOutput(false); // In case we have a serial connection to BrewPi
									   
	// The main purpose of this is to set a boolean value which will allow us to know we
	// just saved a new configuration (as opposed to rebooting normally)
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	// The third parameter we're passing here (mdns_id.c_str()) is the default name that will appear on the form.
	// It's nice, but it means the user gets no actual prompt for what they're entering. 
	WiFiManagerParameter custom_mdns_name("mdns", "Device (mDNS) Name", mdns_id.c_str(), 20);
	wifiManager.addParameter(&custom_mdns_name);

	wifiManager.autoConnect(); // Launch captive portal with auto generated name ESP + ChipID

	// Alright. We're theoretically connected here (or we timed out).
	// If we connected, then let's save the mDNS name
	if (shouldSaveConfig) {
		// If the mDNS name is valid, save it.
		if (isValidmDNSName(custom_mdns_name.getValue())) {
			eepromManager.savemDNSName(custom_mdns_name.getValue());
		} else {
			// If the mDNS name is invalid, reset the WiFi configuration and restart the ESP8266
			WiFi.disconnect(true);
			delay(2000);
			handleReset();
		}
	}

	// Regardless of the above, we need to set the mDNS name and announce it
	if (!MDNS.begin(mdns_id.c_str())) {
		// TODO - Do something about it or log it or something
	}
#endif


#if BREWPI_BUZZER	
	buzzer.init();
	buzzer.beep(2, 500);
#endif	

	piLink.init();

#ifdef ESP8266_WiFi
	// If we're using WiFi, initialize the bridge
	server.begin();
	server.setNoDelay(true);
	// mDNS will stop responding after awhile unless we query the specific service we want
	MDNS.addService("brewpi", "tcp", 23);
	MDNS.addServiceTxt("brewpi", "tcp", "board", "ESP8266");
	MDNS.addServiceTxt("brewpi", "tcp", "branch", "legacy");
	MDNS.addServiceTxt("brewpi", "tcp", "version", VERSION_STRING);
	MDNS.addServiceTxt("brewpi", "tcp", "revision", FIRMWARE_REVISION);
#endif

    bool initialize = !eepromManager.hasSettings();
    if(initialize) {
        eepromManager.zapEeprom();  // Writes all the empty files to SPIFFS
        logInfo(INFO_EEPROM_INITIALIZED);
    }

	logDebug("started");
	tempControl.init();
	settingsManager.loadSettings();

#if BREWPI_SIMULATE
	simulator.step();
	// initialize the filters with the assigned initial temp value
	tempControl.beerSensor->init();
	tempControl.fridgeSensor->init();
#endif	

	display.init();
#ifdef ESP8266_WiFi
	display.printWiFi();  // Print the WiFi info (mDNS name & IP address)
	delay(8000);
	display.clear();
#endif
	display.printStationaryText();
	display.printState();

//	rotaryEncoder.init();

	logDebug("init complete");
}

#ifdef ESP8266_WiFi
void connectClients() {
	if (server.hasClient()) {
		if (!serverClient || !serverClient.connected()) {
			if (serverClient) serverClient.stop();
			serverClient = server.available();
		} else {
			//no free/disconnected spot so reject
			WiFiClient rejectClient = server.available();
			rejectClient.stop();
		}
	}
}

#endif

void brewpiLoop(void)
{
	static unsigned long lastUpdate = 0;
	uint8_t oldState;

	if (ticks.millis() - lastUpdate >= (1000)) { //update settings every second
		lastUpdate = ticks.millis();

#if BREWPI_BUZZER
		buzzer.setActive(alarm.isActive() && !buzzer.isActive());
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
		display.updateBacklight();
	}

	//listen for incoming serial connections while waiting to update
#ifdef ESP8266_WiFi
	yield();
	connectClients();
	yield();
#endif
	piLink.receive();

}

void loop() {
#if BREWPI_SIMULATE
	simulateLoop();
#else
	brewpiLoop();
#endif
}

