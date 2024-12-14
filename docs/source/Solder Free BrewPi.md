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

#### Power Switches

For the solder-free build you will need **one or two** TPLink Kasa power switches, depending on if you want cooling support only, or both cooling and heating. 

* [Kasa Smart Plug EP10](https://www.kasasmart.com/us/products/smart-plugs/kasa-smart-plug-mini-ep10)
* [Kasa Smart Plug HS103 or HS103P2](https://www.amazon.com/gp/product/B07B8W2KHZ/)
* [Kasa Smart Outlet KP200](https://www.amazon.com/gp/product/B07N3CK3MM/)
* [Kasa Smart Power Strip HS300](https://www.amazon.com/Kasa-Smart-Power-Strip-TP-Link/dp/B07G95FFN3/)

There are a number of Kasa plugs that will work, including form factors such as power strips, plug-in dongles, and outlets. You want the traditional WiFi outlets - not the Matter-compatible ones, or the "tapo" ones. 


#### Beer Sensors

Although not strictly required, without a beer sensor BrewPi functions similarly to a traditional thermostat. A beer sensor enables the use of the PID algorithm which leverages the fridge sensor to keep your beer within a range as tight as 0.1 F from the setpoint, as well as features like beer profile mode. The beer sensor needs to be able to sense the temperature of your _beer_, either through the use of a thermowell or by floating within it (in the case of the Tilt Hydrometer).

* [Tilt Pro Hydrometer](https://tilthydrometer.com/products/tilt-pro-wireless-hydrometer-and-thermometer)
* [Inkbird IBS-TH1 Plus](https://www.amazon.com/Inkbird-Bluetooth-Temperature-Thermometer-Hygrometer/dp/B07DQNFJVL/) (Beer and Fridge)
* [Inkbird IBS-TH2 Plus+T](https://www.amazon.com/Inkbird-Thermometer-Temperature-Humidity-Hygrometer/dp/B08TM67HJH/) (Beer and Fridge)

For the Inkbird IBS-TH1 Plus and IBS-TH2 Plus+T, there are two sensors on the device -- one "ambient" temperature sensor that measures the temperature at the device itself, and a second "probe" sensor. These can simultaneously act as the "beer" and "fridge" sensor if the device is mounted inside the fridge, as the ambient sensor can serve as the "fridge" sensor while the probe sensor serves as the "beer" sensor. If you decide to go this route, you do not need one of the fridge sensors listed below. 

**PLEASE NOTE** - For the IBS-TH1 Plus and IBS-TH2 Plus Inkbird offers a "temperature only" probe which looks like a metal cap at the end of a wire, and a "temperatrue + humidity" probe that looks like a plastic grid at the end of a wire. You need the thinner "temperature only" probe. 


#### Fridge Sensors

The "fridge" sensor is required for the operation of a BrewPi device. This sensor measures the air temperature inside the fridge. 

* [Inkbird IBS-TH1 Plus](https://www.amazon.com/Inkbird-Bluetooth-Temperature-Thermometer-Hygrometer/dp/B07DQNFJVL/) (Beer and Fridge)
* [Inkbird IBS-TH2 Plus+T](https://www.amazon.com/Inkbird-Thermometer-Temperature-Humidity-Hygrometer/dp/B08TM67HJH/) (Beer and Fridge)
* [Inkbird IBS-TH2](https://www.amazon.com/Inkbird-Thermometer-Wireless-Bluetooth-Temperature/dp/B08S3CGZ3Q/)


## Instructions

Assembly/setup generally takes 10 minutes or less. To set up your controller, simply follow these steps:

1. If using the case, secure the LoLin D32 Pro into the case bottom using the appropriate screws.
2. Plug the LoLin TFT Screen into the D32 Pro using the LoLin TFT Cable
3. If using the case, insert the screen/cable into the case, and secure the case lid to the case base using the appropriate screws
4. Flash the BrewPi-ESP WiFi firmware to your D32 Pro using [these instructions](Installing%20the%20Firmware.md)
5. Using a phone or other WiFi device, connect to the "BrewPiAP" WiFi network that your controller creates (password is "brewpiesp") and connect it to your WiFi network
5. Connect the Kasa Smart Plug to the same WiFi network as your BrewPi by following the instructions included with the switches
6. Using a phone or other WiFi device connected to the same WiFi network, log into the web interface by typing the IP address displayed in the lower left corner of the BrewPi screen into a web browser
7. Click "Set Up Sensors/Actuators" and assign the appropriate functions to your sensors and switches
8. Connect your controller to your installation of [Fermentrack](http://www.fermentrack.com/), [Fermentrack.net](https://www.fermentrack.net/), or [BrewPi Remix](http://www.brewpiremix.com/)

Enjoy your new BrewPi temperature controller!
