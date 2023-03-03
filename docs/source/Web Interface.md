# Using the BrewPi-ESP Web Interface


## Dashboard

The dashboard contains a "virtual" LCD showing the state of the controller, as well as dashboard panels breaking out the various control points. From this panel you can also change the control mode the controller is using to manage temperatures by clicking the "Change" link on the Control Mode panel.


### Changing Control Mode

After clicking the "Change" link a window will pop up allowing you to select the new control mode. There are three selectable modes from the web interface, and one mode that may appear if the controller is connected to BrewPi Remix or Fermentrack:

* **Off** - Temperature control is disabled, and cooling/heating will be turned off
* **Fridge Constant** - The controller will control the fridge/chamber temperature to match a setpoint
* **Beer Constant** - The controller will control the beer temperature, using the fridge temperature as an input to the algorithm
* **Beer Profile** - _Not selectable from the web interface_ - If the controller is connected to BrewPi Remix or Fermentrack and is currently applying a fermentation temperature profile, the controller will be placed in Beer Profile mode. Beer Profile mode is not selectable from the web interface as management of the profile is handled by Fermentrack/BrewPi Remix.

When selecting Fridge Constant or Beer Constant mode you will be asked to provide a temperature setpoint to control to in the temperature units selected on the controller.


## Set up Sensors/Actuators

BrewPi-ESP is designed to read temperatures from a number of different types of temperature data sources and use that to control temperature by actuating several types of switches. In order for this to work, BrewPi-ESP must be told which temperature sensors correspond with each data type (beer temp, fridge temp, or room temp) and which actuators correspond to heating/cooling.  The Set Up Sensors/Actuators page allows you to see all sensors/actuators known to the controller and assign each device the relevant function. 

More information on supported sensors and actuators can be found in [](Supported Sensors & Actuators.md).


## Controller Settings

The Controller Settings page allows you to toggle certain settings associated with the controller. A discussion of these settings can be found in [](Controller Configuration Options.md).


## About Controller

The About Controller page allows you to see the controller's firmware version, the time since last reboot, some debugging information around what caused the last reboot, and the amount of free memory available on the controller. 


## Technical Notes

The BrewPi-ESP front end is written in JavaScript + Vue. The source files for the interface are saved to a [separate repository on GitHub](https://github.com/thorrak/brewpi_esp_ui) and are governed by a different license from BrewPi-ESP. The front end primarily speaks to the controller via a number of JSON api endpoints that were designed specifically for this purpose. Please be aware that these endpoints are not expected to remain static between firmware versions, and _will_ change without notice.
s