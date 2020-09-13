# Installing the Firmware

## BrewFlasher & Fermentrack

The easiest way to install the firmware is using either [BrewFlasher](http://www.brewflasher.com/) or the firmware flow in [Fermentrack](http://www.fermentrack.com/). 

BrewFlasher is a standalone application for MacOS and Windows that allows for the firmware to be automatically downloaded & flashed to your controller. It was specifically designed to make flashing *this* project easy. For most users, this is the ideal installation method.

Fermentrack is a web interface that BrewPi controllers can connect to/be managed by. If you were already planning on using Fermentrack to manage your BrewPi controller then this is a perfect alternative to either using BrewFlasher or flashing manually. 

## Manual Installation Methods

### Linux/Raspberry Pi - Manual
The easiest way is to use the custom script in my brewpi-tools fork, though the script only completes steps 1 & 4 of the below. Alternatively, you can install this manually by doing the following:

1. Install esptool using PIP (`pip install esptool`)
2. Hook up the ESP8266 to your Raspberry Pi with a USB cable
3. Locate the USB serial bridge device. Generally this will be `/dev/ttyUSB0`, however if there is any question you can see a mapping of all USB serial devices by looking in `/dev/serial/by-id`.
4. Download the repo to your Raspberry Pi using `git clone`
5. Change to the `bin` directory (`cd /home/brewpi/esp8266/bin`, or the appropriate directory)
6. Flash the firmware (`esptool --port /dev/ttyUSB0 write_flash -fm=dio -fs=32m 0x00000 /home/brewpi/esp8266/bin/brewpi-esp8266.v0.6.wifi.bin`)

*Note:*  If you receive an error stating `command not found` when flashing the firmware, it may be that the esptool is not in your path.  Use the following command with explicit paths:

`python /usr/local/lib/python2.7/dist-packages/esptool.py --port /dev/ttyUSB0 write_flash -fm=dio -fs=32m 0x00000 /home/brewpi/esp8266/bin/brewpi-esp8266.v0.11.wifi.bin`


## macOS
1. If you are using macOS 10.13 or lower, then you will likely have to install a driver for the USB serial chip that is on most ESP8266 D1 minis. You can download it from here: https://kig.re/downloads/CH341SER_MAC.ZIP . macOS 10.14 Mojave and higher do not require this driver, and installing it may cause issues on your system.
2. Install a version of Python 3.x from https://www.python.org/downloads/
3. Install esptool using PIP (`pip3 install esptool`)
4. Hook up the ESP8266 to your Mac with a USB cable
5. Locate the USB serial bridge device. Generally this will be `/dev/tty.wchusbserial1410` or something similar.
6. Download the repo to your Mac using `git clone`
7. Change to the `bin` directory (`cd ~/Downloads/brewpi-esp8266/bin`, or the appropriate directory)
8. Flash the firmware (`python3  /Library/Frameworks/Python.framework/Versions/3.7/lib/python3.7/site-packages/esptool.py --port /dev/tty.wchusbserial1410 write_flash -fm=dio 0x00000 brewpi-esp8266.v0.11.wifi.bin`)


You can probably install this using esptool similar to the instructions above.


## Windows
The esp8266 modules may be flashed with the NodeMCU Flasher for Windows:
https://github.com/nodemcu/nodemcu-flasher

Download from the Win32/Release or Win64/Release folder in the repository as appropriate for your architecture.
