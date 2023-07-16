# Building a "Solder Free" Brewpi

Through the combination of the ESP32 firmware, Bluetooth support, and Kasa WiFi Switch support, it is now possible to build a BrewPi temperature controller without the need to solder anything. Here's how to do it:



## Bill of Materials

### Controller

* 1 x [LoLin D32 Pro](https://www.aliexpress.us/item/2251832696801305.html)
* 1 x [LoLin TFT Screen](https://www.aliexpress.us/item/2251832733414978.html)
* 1 x [LoLin TFT Cable](https://www.aliexpress.us/item/2251832662518722.html)
* 1 x [Case (including screws)](https://github.com/thorrak/thorrak_hardware/tree/master/TiltBridge%20Containers/D32%20Pro%20Container)

Although any ESP32 board can be used, and the screen/case are not mandatory, I find that the above hardware in combination with a 3D printed case works perfectly.


### Other Components

* 1-2 x [Kasa Smart Plug](https://www.kasasmart.com/us/products/smart-plugs/kasa-smart-plug-mini-ep10) (Heat/Cool Switches)
* 1 x [Inkbird IBS-TH2](https://www.amazon.com/Inkbird-Thermometer-Wireless-Bluetooth-Temperature/dp/B08S3CGZ3Q/) (Fridge Temp Sensor)
* 1 x [Tilt Pro Hydrometer](https://tilthydrometer.com/products/tilt-pro-wireless-hydrometer-and-thermometer) (Beer Temp Sensor)
* 1 x [Inkbird IBS-TH1 Plus](https://www.amazon.com/Inkbird-Bluetooth-Temperature-Thermometer-Hygrometer/dp/B07DQNFJVL/) (Beer Temp Sensor)

You only need one of the Tilt Pro or Inkbird IBS-TH1 Plus to act as a beer temperature sensor -- The Tilt Pro is inserted directly into your beer and will measure the temperature floating on top, or you can use an Inkbird IBS-TH1 Plus which has a wired temperature sensor that can be snaked into a thermowell. Although regular, non-pro Tilts can be used, I do not recommend them as the thermometer only measures in whole-degree increments.

The EP10 Mini Kasa Smart Plug linked above supports loads up to 15A sustained, which should be sufficient for most heating/cooling setups -- please be sure not to overload the switch.


## Instructions

Assembly/setup generally takes 10 minutes or less. To set up your controller, simply follow these steps:

1. If using the case, secure the LoLin D32 Pro into the case bottom using the appropriate screws.
2. Plug the LoLin TFT Screen into the D32 Pro using the LoLin TFT Cable
3. If using the case, insert the screen/cable into the case, and secure the case lid to the case base using the appropriate screws
4. Flash the BrewPi-ESP WiFi firmware to your D32 Pro using [these instructions](Installing%20the%20Firmware.md)
5. Using a phone or other WiFi device, connect to the "BrewPiAP" WiFi network that your controller creates and connect it to your WiFi network
5. Connect the Kasa Smart Plug to the same WiFi network as your BrewPi by following the instructions included with the switches
6. Using a phone or other WiFi device connected to the same WiFi network, log into the web interface by typing the IP address displayed in the lower left corner of the BrewPi screen into a web browser
7. Click "Set Up Sensors/Actuators" and assign the appropriate functions to your Kasa switches, Inkbird temperature sensor, and Tilt hydrometer
8. Connect your controller to your installation of [Fermentrack](http://www.fermentrack.com/) or [BrewPi Remix](http://www.brewpiremix.com/)

Enjoy your new BrewPi temperature controller!
