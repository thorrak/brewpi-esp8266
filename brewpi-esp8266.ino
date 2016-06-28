#include <EEPROM.h>
#include <OneWire.h>
//#include "Brewpi.h"

// setup and loop are in brewpi_config so they can be reused across projects
//extern void setup(void);
//extern void loop(void);


void handleReset()
{
	// resetting using the watchdog timer (which is a full reset of all registers) 
	// might not be compatible with old Arduino bootloaders. jumping to 0 is safer.
#if defined(ESP8266)
	// The asm volatile method doesn't work on ESP8266. Instead, use ESP.restart
	ESP.restart();
#else
	asm volatile ("  jmp 0");
#endif
}
/*
#ifndef ESP8266
// TODO - Determine if this actually is required
void main() __attribute__((noreturn)); // tell the compiler main doesn't return.
#endif

#if defined(ESP8266)
int main(void)
#else
void main(void)
#endif
{
init();

#if defined(USBCON)
USBDevice.attach();
#endif

setup();

for (;;) {
loop();
//		if (serialEventRun) serialEventRun();
}

#if defined(ESP8266)
return 0;
#endif
}

// catch bad interrupts here, uncomment while only when debugging
//ISR(BADISR_vect){
;//while (1);
//}

*/

/*
void setup() {
	pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

// the loop function runs over and over again forever
void loop() {
	digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
									  // but actually the LED is on; this is because 
									  // it is acive low on the ESP-01)
	delay(1000);                      // Wait for a second
	digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
	delay(2000);                      // Wait for two seconds (to demonstrate the active low LED)
}
*/
#include "Brewpi.h"
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



//void loop (void);

/* Configure the counter and delay timer. The actual type of these will vary depending upon the environment.
* They are non-virtual to keep code size minimal, so typedefs and preprocessing are used to select the actual compile-time type used. */
TicksImpl ticks = TicksImpl(TICKS_IMPL_CONFIG);
DelayImpl wait = DelayImpl(DELAY_IMPL_CONFIG);

DisplayType realDisplay;
DisplayType DISPLAY_REF display = realDisplay;

ValueActuator alarm;

void setup()
{

#if defined(ESP8266)
	// We need to initialize the EEPROM on ESP8266
	EEPROM.begin(MAX_EEPROM_SIZE_LIMIT);
#endif

#if BREWPI_BUZZER	
	buzzer.init();
	buzzer.beep(2, 500);
#endif	

	piLink.init();

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
	display.printStationaryText();
	display.printState();

	rotaryEncoder.init();

	logDebug("init complete");
}


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
	piLink.receive();

}

void loop() {
#if BREWPI_SIMULATE
	simulateLoop();
#else
	brewpiLoop();
#endif
}

