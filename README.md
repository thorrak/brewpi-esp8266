# brewpi-esp8266
An implementation of the BrewPi device code on an ESP8266 (No arduino needed!) /w bonus WiFi support

## Introduction
This project seeks to implement the BrewPi firmware onto an ESP8266 controller. It is compatible with both [Fermentrack](http://www.fermentrack.com/) as well as the legacy BrewPi-www software.

### Documentation:
* [Selecting an ESP8266 Board](docs/Selecting%20an%20ESP%20Board.md)  
* [Full Installation Instructions](docs/INSTALL.md) (including custom brewpi-script)
* [Installing the Firmware](docs/Installing%20the%20Firmware.md)
* [Developing/Contributing](docs/DEVELOP.md)
* [Hardware Information](https://github.com/thorrak/thorrak_hardware/blob/master/BrewPi-ESP8266.md) - PCB files, ordering and BOMs

## Expected Pinout
* D0 - Heat
* D1 - I2C SCL
* D2 - I2C SDA
* D3 - Buzzer *(currently unsupported)*
* D4 - N/C
* D5 - Cool
* D6 - OneWire Data
* D7 - Door (Untested)

## Supported Features
* OneWire Temperature Sensors
* SPIFFS configuration saving
* Pin-Based Relays
* WiFi Soft-AP Configuration
* OTA (WiFi) connection to BrewPi 
* I2C LCD (20x4) Screen (/w Address Autodetection)

## Currently Unsupported/Untested
* Rotary Encoder Support
* Door Sensor Support (Note - May work, untested)
* Buzzer Support
* OneWire "actuators"

## Differences vs. Arduino Implementation
The primary goal in creating this was to mirror the spirit of the Arduino implementation as closely as possible while adding in support for WiFi. That said, where I've needed to decide between preserving the original code and adapting for use with the ESP8266 I've generally erred towards the latter. This is especially true with the EEPROM code - this implementation completely replaces the EEPROM with SPIFFS. 

When possible, I've broken out ESP8266 specific code using preprocessor logic. Although this should in theory allow this code to be compiled against an Arduino target, backwards compatibility is not guaranteed. The primary goal of using preprocessor logic is to document changes from the Arduino codebase - not to maintain it.

Due to the fact that this is an unsupported board, I have had to rewrite a small portion of brewpi-script to support it. I've also merged in (some of the) changes that were proposed to the official repo late last year to support network sockets (and therefore, WiFi). As a result, to use brewpi-esp8266 you will need to also use this modified brewpi-script. 

## Fermentrack Integration

Although this firmware works with brewpi-www, it is highly recommended that users use [Fermentrack](http://www.fermentrack.com/) instead as Fermentrack substantially reduces the work required to get an ESP8266-based controller working. Due to recent changes in Raspbian, ongoing testing & support for this firmware is exclusively performed on Fermentrack and future changes may introduce incompatibility with brewpi-www.


