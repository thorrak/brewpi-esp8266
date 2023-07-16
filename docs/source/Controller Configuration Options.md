# Controller Configuration Options


## User Guide to Configuration

The majority of the configuration of your BrewPi happens behind-the-scenes, either handled automatically by the controller itself or managed by Fermentrack or BrewPi Remix.


### "Low Delay" Mode

BrewPi implements a number of protections designed to prevent damage to compressor-based cooling systems such as refrigerators, kegerators, or keezers. These include requiring cooling to run for a minimum period of time before being turned off, requiring a minimum amount of time to pass with the cooling off before allowing it to come back on, and requiring a minimum amount of time to pass before switching between heat & cooling. Low Delay mode reduces the delays associated with cooling

|                                                | **Normal** | **"Low Delay"** |
|------------------------------------------------|------------|-----------------|
| Minimum Cooling "Off" Time                     | 300s       | 60s             |
| Minimum Heat "Off" Time                        | 300s       | 300s            |
| Minimum Cooling "On" Time                      | 180s       | 20s             |
| Minimum Heat "On" Time                         | 180s       | 180s            |
| Min Cool "Off" Time (Fridge Constant Mode)     | 600s       | 60s             |
| Min time between switching between heat & cool | 600s       | 600s            |


:::{Warning}
Low Delay mode should never be used with compressor-based cooling as it may damage your equipment. Low Delay mode is intended for other types of cooling, such as glycol, chilled water pumps, fans/ice, or peltier coolers.
:::


### Rotate TFT

TFT-based builds (ESP32) can have the display rotated if it is inserted in the case upside down (or the manufacturer has changed the default orientation). If your screen is upside-down, toggle this on. 




