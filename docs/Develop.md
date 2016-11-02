#Developing/Compiling brewpi-esp8266


##Introduction

So you want to help develop brewpi-esp8266? Great! There are plenty of features
which are not currently implemented, bugs left unsquished, optimizations left
unoptimized - the sky's the limit.


## Required Tools
You will need:
* [Platformio](http://docs.platformio.org/en/stable/installation.html#installation-methods)
* A [supported](http://docs.platformio.org/en/stable/ide.html) IDE (I prefer [CLion](https://www.jetbrains.com/clion/), but the free [Platformio IDE](http://platformio.org/platformio-ide) should also work)

# Cloning the Repo & Configuring Platformio
Once you've set up the tools above, you will need to clone the GitHub repo. Once cloned, you will need to configure Platformio. To do this, open a command line client & change to <repo directory>/legacy-platformio/
Then run `platformio init --board=nodemcuv2 --ide=clion` (replacing the ide name with your IDE of choice)

## Required Arduino Libraries
One key change from the original Arduino version of the firmware is that I
went back to using contributed libraries in favor of local copies when
possible. That said, Platformio supports libraries in the project directory (and as such, I've attempted to copy libraries there when available)

## Key differences from the Arduino version
One key difference to be aware of is that unlike the Arduino the ESP8266 doesn't
contain a dedicated EEPROM. I used to get around this using EEPROM emulation, but lets be honest - SPIFFS on the ESP8266 is far more flexible, with load-leveling should last longer, and just feels more modern overall. I've adjusted things to use SPIFFS rather than the EEPROM and have altered the EEPROM code accordingly.
