# Installing the Firmware

## BrewFlasher & Fermentrack

The easiest way to install the firmware is using either [BrewFlasher](http://www.brewflasher.com/), [BrewFlasher Web Edition](https://web.brewflasher.com/) or the firmware flow in [Fermentrack](http://www.fermentrack.com/). 

BrewFlasher is a standalone application for MacOS and Windows that allows for the firmware to be automatically downloaded & flashed to your controller. It was specifically designed to make flashing *this* project easy. For most users, this is the ideal installation method.

BrewFlasher Web Edition is a browser-based version of BrewFlasher that allows you to install firmware using a compatible web browser without needing to download or install the desktop version of BrewFlasher. It supports the same list of firmware as BrewFlasher, but requires that you  

Fermentrack is a web interface that BrewPi controllers can connect to/be managed by. If you were already planning on using Fermentrack to manage your BrewPi controller then this is a perfect alternative to either using BrewFlasher or flashing manually. 


## esptool

The firmware can also be installed using esptool (which is what BrewFlasher uses behind-the-scenes). This is not recommended for most users, as you will need to craft the correct command to flash the correct files to the proper offsets, which will change depending on the specific firmware you are looking to flash. 



## NodeMCU Flasher for Windows (ESP8266-only)

ESP8266 modules can also be flashed using [NodeMCU-PyFlasher](https://github.com/marcelstoer/nodemcu-pyflasher).
