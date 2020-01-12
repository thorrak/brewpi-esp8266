# Hardware

To build a working ESP-based BrewPi controller, you will generally need 6 main components:

* Main Controller PCB (see below)
* Sensor breakout PCB (see below)
* LCD2004 I2C LCD Screen
* 2-channel Relay Board
* 3x DS18B20 Temp Sensors (non-parasitic mode)
* An enclosure of some sort


To assemble the controller, you will also need:

* Elecrical Wire
* Electrical Outlet
* Wire Nuts
* Female-to-Female Dupont Cables
* Cable to connect the sensor PCB to the main PCB (generally RJ-45 ethernet cable)


## BrewPi ESP8266 PCB Information

In general, building an ESP8266-based BrewPi is greatly simplified by using a
PCB which does most of the wiring for you. For the purposes of this document,
we're splitting the build into two parts - the "Main PCB" to which the
ESP8266 is connected, and the "Sensor Breakout" which connects to the
temperature and (optionally) door sensor.


* To compare options for the Main PCB for the ESP8266, [click here](ESP8266%20BrewPi%20Boards/README.md).
* To compare options for the sensor breakout boards, [click here](BrewPi%20Sensor%20Boards/README.md).


