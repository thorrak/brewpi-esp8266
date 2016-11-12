# Installing the Firmware

## Linux/Raspberry Pi
The easiest way is to use the custom script in my brewpi-tools fork, though the script only completes steps 1 & 4 of the below. Alternatively, you can install this manually by doing the following:

1. Install esptool using PIP (`pip install esptool`)
2. Hook up the ESP8266 to your Raspberry Pi with a USB cable
3. Locate the USB serial bridge device. Generally this will be `/dev/ttyUSB0`, however if there is any question you can see a mapping of all USB serial devices by looking in `/dev/serial/by-id`.
4. Download the repo to your Raspberry Pi using `git clone`
5. Change to the `bin` directory (`cd /home/brewpi/esp8266/bin`, or the appropriate directory)
6. Flash the firmware (`esptool --port /dev/ttyUSB0 write_flash -fm=dio -fs=32m 0x00000 /home/brewpi/esp8266/bin/brewpi-esp8266.v0.6.wifi.bin`)

*Note:*  If you receive an error stating `command not found` when flashing the firmware, it may be that the esptool is not in your path.  Use the following command with explicit paths:

`python /usr/local/lib/python2.7/dist-packages/esptool.py --port /dev/ttyUSB0 write_flash -fm=dio -fs=32m 0x00000 /home/brewpi/esp8266/bin/brewpi-esp8266.v0.6.wifi.bin`


## Mac OS X
*To be updated*

You can probably install this using esptool similar to the instructions above.


## Windows
The esp8266 modules may be flashed with the NodeMCU Flasher for Windows:
https://github.com/nodemcu/nodemcu-flasher

Download from the Win32/Release or Win64/Release folder in the repository as appropriate for your architecture.
