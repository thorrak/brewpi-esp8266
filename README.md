# brewpi-esp8266
An implementation of the BrewPi device code on an ESP8266 (No arduino needed!) /w bonus WiFi support

## Introduction


## Supported Features
* OneWire Temperature Sensors
* EEPROM configuration saving
* Relay
* WiFi Soft-AP Configuration
* OTA (WiFi) connection to BrewPi 

## Currently Unsupported/Untested
* LCD Support
* Rotary Encoder Support
* Door Sensor Support (Note - May work, untested)
* Buzzer Support
* OneWire "actuators"

## Differences vs. Arduino Implementation
The primary goal in creating this was to mirror the spirit of the Arduino implementation as closely as possible while adding in support for WiFi. That said, where I've needed to decide between preserving the original code and adapting for use with the ESP8266 I've generally erred towards the latter.

When possible, I've broken out ESP8266 specific code using preprocessor logic. Although this should in theory allow this code to be compiled against an Arduino target, backwards compatibility is not guaranteed. The primary goal of using preprocessor logic is to document changes from the Arduino codebase - not to maintain it. 

Due to the fact that this is an unsupported board, I have had to rewrite a small portion of brewpi-script to support it. I've also merged in (some of the) changes that were proposed to the official repo late last year to support network sockets (and therefore, WiFi). As a result, to use brewpi-esp8266 you will need to also use this modified brewpi-script. 
