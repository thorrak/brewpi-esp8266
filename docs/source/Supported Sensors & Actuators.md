# Supported Temperature Sensors & Switches/Actuators


## Actuators (Switches/Relays)

Two types of actuators are currently supported: Pin-based and Kasa WiFi Switches:

### Pin-based Actuators

Pin-based actuators are the traditional type used with BrewPi where an output pin on the controller turns on/off based on whether the controller is calling for heating or cooling. This pin is typically then connected to a relay which then switches the load of the heater/cooler. The "sainsmart" style relay boards are slightly unintuitive in that the pin being "high" (on) actually results in the load being turned _off_. To resolve this, there is an "Invert Pin" option that can be set to ensure that these style of boards are properly toggled on/off. 

:::{important}
When setting up a pin, make sure that the device is off when you expect it to be off, and is on when you expect it to be on. Setting the "invert" flag incorrectly can reverse these, resulting in frozen beer (or worse, if your heater runs indefinitely!)
:::

### TPLink Kasa WiFi Smart Switches

Kasa WiFi Switches are "smart" plugs which allow for your cooling/heating source to be plugged into a WiFi-enabled device that can then be toggled over your network. BrewPi-ESP detects these devices when connected to the same WiFi network, and can control them without using TP Link's cloud. Initial connection of these devices to the internet (along with initial setup) is completed in the Kasa smart phone app -- once complete, these devices should be detected by BrewPi-ESP.


## Temperature Sensors

There are three types of temperature sensors supported by BrewPi-ESP: OneWire (DS18b20) temperature sensors, Inkbird bluetooth temperature sensors, and Tilt Pro hydrometers. All three types of temperature sensors allow for a calibration offset to be defined: An amount - specified in 1/16 degree C - that will be added to/subtracted from each reading.


### OneWire Sensors (DS18b20)

OneWire (DS18b20) temperature sensors are the type traditionally used with BrewPi, and are hard-wired to the controller. They can often be purchased in stainless, waterproof housing, connected to several meter long cables, ready for insertion into a thermowell.  Most of these sensors are powered via discrete power, but parasite power versions exist -- BrewPi-ESP will not function with parasitic sensors.


### Inkbird Bluetooth Temperature Sensors

Inkbird manufactures a number of wireless bluetooth temperature sensors, several of which have been tested and work with BrewPi-ESP:

* Inkbird IBS-TH1 Plus
* Inkbird IBS-TH2 (both temperature only and temperature + humidity)
* Inkbird IBS-TH2 Plus

The "plus" versions of these sensors typically come with an extendable temperature probe that can be inserted into a thermowell, and oftentimes also include a display.


### Tilt Hydrometers

Tilt hydrometers are bluetooth devices that measure both the specific gravity and temperature of your fermenting beer. There are two versions available - the Tilt and the Tilt Pro. The Tilt Pro offers temperature resolution of 0.1 F whereas the regular Tilt offers temperature resolution of whole degrees fahrenheit. The Tilt is an ideal solution for setups that lack a thermowell (or instances where the Tilt would be utilized regardless). 

:::{note}
Due to their higher thermal mass and finer resolution, temperature control using a Tilt is only recommended with the Tilt Pro. Use of a non-Pro tilt is possible, but will result in less accurate temperature control. 
:::

