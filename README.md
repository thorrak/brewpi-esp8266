# BrewPi-ESP
An implementation of the BrewPi firmware on the ESP8266, ESP32, and ESP32-S2 with additional, added features

## Introduction
BrewPi provides a temperature controller which uses PID algorithms to tightly control a fermenting beer's temperature -- meaning that you can in many cases control the temperature of your fermenting beer to within _one tenth of a degree_. It was originally designed by Elco Jacobs, and used an Arduino running the BrewPi firmware to measure temperatures and toggle heating/cooling, paired with a Raspberry Pi to receive & graph temperature changes over time.

This project ports the BrewPi firmware to the ESP8266, ESP32, and ESP32-S2 controllers, eliminating the need for an Arduino. It is compatible with [Fermentrack](http://www.fermentrack.com/) and [BrewPi-Remix](https://www.brewpiremix.com/) - both of which can be installed on a Raspbery Pi to capture/graph temperatures similar to the original project.

### Documentation:
* [Selecting your Hardware](docs/source/Selecting%20an%20ESP%20Board.md)  
* [Building a "Solder Free" BrewPi](docs/source/Solder%20Free%20BrewPi.md)  
* [Installing the Firmware](docs/source/Installing%20the%20Firmware.md)
* [Hardware Information](https://github.com/thorrak/thorrak_hardware/blob/master/BrewPi-ESP8266.md) - PCB files, ordering and BOMs

## Configuration
Once you've flashed firmware to your ESP device, you'll need to configure it to connect to your WiFi network.  For that purpose, the firmware creates a WiFi access point named `BrewPiAP`, with a password of `brewpiesp`.  Connect to that access point and, if your device doesn't pull up a configuration page on its own, browse to `http://192.168.4.1`.  Then click on **Configure WiFi,** and configure it to connect to your WiFi network.

## Expected Pinout

### ESP32

There are two versions of the ESP32 firmware - one supporting an LCD2004 I2C display and one supporting a TFT. The pinout is slightly different between the two:

#### Common Pins

* 25 - Heat
* 26 - Cool
* 13 - OneWire Data
* 34 - Door

#### I2C Pins

* 21 - I2C SDA
* 22 - I2C SCL

#### TFT Pins

* 14 - TFT CS
* 27 - TFT DC
* 33 - TFT RST
* 12 - TS CS
* 32 - TFT Backlight

### ESP32-S2

* 5 - Heat
* 7 - Cool
* 9 - OneWire Data
* 11 - Door
* 33 - I2C SDA
* 35 - I2C SCL


### ESP8266

Although the ESP8266 was the original board chosen for this project, due to reduced support by the manufacturer its use in new builds is not recommended. For existing builds using the LoLin D1 Mini, the LoLin S2 Mini (utilizing the ESP32-S2) is recommended as a pin compatible replacement.

* D0 - Heat
* D1 - I2C SCL
* D2 - I2C SDA
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
* Tilt Hydrometer (as Temp Sensor) (ESP32 only)
* TPLink Kasa WiFi Switches (ESP32 & ESP32-S2 WiFi only)
* HTTP interface for quick updates

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

## Hardware

I maintain a separate repo with PCB and 3D-printable case designs for all of my projects. The page specific to BrewPi-ESP can be accessed [here](https://github.com/thorrak/thorrak_hardware/blob/master/BrewPi-ESP8266.md). 
