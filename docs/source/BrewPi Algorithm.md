# BrewPi Algorithm

BrewPi uses a (PID algorithm)[https://en.wikipedia.org/wiki/PID_controller] to control the fermentation temperature of beer very precisely. The algorithm works by adjusting the temperature setting of the fridge a fermenter is in based on the desired temperature of the beer. This is done by an automated process, similar to stationing a robot next to your fridge's thermostat and telling it to adjust the thermostat up or down based on where you think it needs to be in order to get the beer temperature correct.

Just as when typically adjusting the thermostat on a refrigerator, whether or not the fridge turns on takes into account factors such as how far the current temperature is from the setpoint and how long it has been since the last cooling cycle took place (compressor protection). Heating or cooling will take place until the fridge temperature meets or surpasses the setpoint (though similar to the built in thermostat, there may be a "minimum run time" so as to protect the compressor). When using a heater the algorithm continues to simply change the fridge temperature setpoint - toggling cooling or heating depending on whether the fridge temperature is above or below the desired fridge setpoint.

The BrewPi algorithm determines the optimal fridge setpoint based on the beer temperature and how historically changing the fridge thermostat has affected the beer temperature. The PID algorithm measures error in the 

However, the algorithm has limits, as it uses a single algorithm to control both heating and cooling. This assumption assumes that both heating and cooling will be impacted by the same degree of error which means that you will generally want to match your heating and cooling power, as well as the degree to which the heating/cooling can be picked up by the fridge sensor.

