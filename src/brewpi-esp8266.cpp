// Libraries have to be loaded in the main .ino file per Visual Micro. Load them here.

#ifdef ESP8266
#include <FS.h>  // Apparently this needs to be first
#endif

#include "Brewpi.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>		//ESP8266 Core WiFi Library (always included so we can disable the radio for serial)

#ifdef ESP8266_WiFi
#include <ESP8266mDNS.h>
#include <DNSServer.h>			//Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>	//Local WebServer used to serve the configuration portal
#include <WiFiManager.h>		//https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "Version.h" 			// Used in mDNS announce string
#endif
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

WiFiEventHandler stationConnectedHandler;
void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {
    server.begin();
    server.setNoDelay(true);
}


#endif

void handleReset()
{
	// The asm volatile method doesn't work on ESP8266. Instead, use ESP.restart
	ESP.restart();
}



void setup()
{
    // Let's get the display going so that we can provide the user a bit of feedback on what's happening
    display.init();
    display.printEEPROMStartup();

    // Before anything else, let's get SPIFFS working. We need to start it up, and then test if the file system was
    // formatted.
	SPIFFS.begin();

    if (!SPIFFS.exists("/formatComplete.txt")) {
        if (!SPIFFS.exists("/mdns.txt")) {
            // This prevents installations that are being upgraded from v0.10 to v0.11 from having their mdns settings
            // wiped out
            SPIFFS.format();
        }
        File f = SPIFFS.open("/formatComplete.txt", "w");
        f.close();
    }

#ifdef ESP8266_WiFi
    display.printWiFiStartup();
	String mdns_id;

	mdns_id = eepromManager.fetchmDNSName();
//	if(mdns_id.length()<=0)
//		mdns_id = "ESP" + String(ESP.getChipId());


	// If we're going to set up WiFi, let's get to it
	WiFiManager wifiManager;
	wifiManager.setConfigPortalTimeout(5*60); // Time out after 5 minutes so that we can keep managing temps 
	wifiManager.setDebugOutput(FALSE); // In case we have a serial connection to BrewPi
									   
	// The main purpose of this is to set a boolean value which will allow us to know we
	// just saved a new configuration (as opposed to rebooting normally)
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	// The third parameter we're passing here (mdns_id.c_str()) is the default name that will appear on the form.
	// It's nice, but it means the user gets no actual prompt for what they're entering. 
	WiFiManagerParameter custom_mdns_name("mdns", "Device (mDNS) Name", mdns_id.c_str(), 20);
	wifiManager.addParameter(&custom_mdns_name);

//	wifiManager.autoConnect("ESP-BrewPi-Setup"); // Launch captive portal with static name
    // Launch captive portal with auto generated name ESP + ChipID
    if(wifiManager.autoConnect()) {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_AP_STA);
    } else {
        // We failed to connect - turn WiFi off
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
    }

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
#else
    // Apparently, the WiFi radio is managed by the bootloader, so not including the libraries isn't the same as
    // disabling WiFi. We'll explicitly disable it if we're running in "serial" mode
	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
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

	// This code no longer really does anything.
//    bool initialize = !eepromManager.hasSettings();
//    if(initialize) {
//        eepromManager.zapEeprom();  // Writes all the empty files to SPIFFS
//        logInfo(INFO_EEPROM_INITIALIZED);
//    }

	logDebug("started");
	tempControl.init();
	settingsManager.loadSettings();

#if BREWPI_SIMULATE
	simulator.step();
	// initialize the filters with the assigned initial temp value
	tempControl.beerSensor->init();
	tempControl.fridgeSensor->init();
#endif	

#ifdef ESP8266_WiFi
	display.printWiFi();  // Print the WiFi info (mDNS name & IP address)
    WiFi.setAutoReconnect(true);
    stationConnectedHandler = WiFi.onSoftAPModeStationConnected(&onStationConnected);
	delay(8000);
	display.clear();
#endif
	display.printStationaryText();
	display.printState();

//	rotaryEncoder.init();

	logDebug("init complete");
}

#ifdef ESP8266_WiFi
	int reconnectPoll = 0;
	void connectClients() {

    if(WiFi.isConnected()) {
        if (server.hasClient()) {
            // If we show a client as already being disconnected, force a disconnect
            if (serverClient) serverClient.stop();
            serverClient = server.available();
            serverClient.flush();
        }
    } else {
        // This might be unnecessary, but let's go ahead and disconnect any "clients" we show as connected given that
        // WiFi isn't connected
		// If we show a client as already being disconnected, force a disconnect
		if (serverClient) {
			serverClient.stop();
			serverClient = server.available();
			serverClient.flush();
		}
    }
}

#endif

void brewpiLoop(void)
{
	static unsigned long lastUpdate = 0;
    static unsigned long lastLcdUpdate = 0;

	uint8_t oldState;
    if(ticks.millis() - lastLcdUpdate >= (180000)) { //reset lcd every 180 seconds as a workaround for screen scramble
        lastLcdUpdate = ticks.millis();

        display.init();
        display.printStationaryText();
        display.printState();

        rotaryEncoder.init();
    }

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

