# Installing brewpi-esp8266

There are two main methods for installing brewpi-esp8266:
* Full installation alongside BrewPi
* Post-Brewpi Installation



## Installation Instructions
1. Prepare the Raspberry Pi (install Raspbian, run raspi-config, update/upgrade)
2. Install BrewPi using the custom brewpi-tools.
3. Flash the firmware to the ESP8266 board.
4. Set up the firmware
5. Configure BrewPi to use your firmware


### 3. Flash the firmware to the ESP8266 board.
As there are a number of ways to do this (depending on your operating system,
the board you are using, etc.) these instructions are located in a
[different file](Installing the Firmware.md). Go read them, and come back.



### 4. Set up the firmware
Once you have flashed the firmware to the ESP8266 board, you will need to
configure it for use. If you are using the "serial" version of the firmware
then congratulations - you're done! For the WiFi version, you will need to
configure access to your wireless network.

1. From a wifi-enabled device (computer, phone, etc.) look for a new access point named "ESPXXXXXXX" where XXXXX is a number.
2. Connect to this access point and open a web browser. (Some devices may automatically open one)
3. **Write down the access point name. (ESPXXXXXX) You will need this later.**
3. Click "Scan for WiFi"
4. Select your wireless network & enter your password
5. Click "connect"


### 5. Configure BrewPi to use your firmware
Now that you've set up the firmware you need to point the BrewPi script at it.
1. Log into the Raspberry Pi, and change user to brewpi (`sudo su brewpi`)
2. Copy the default configuration file (`cp ~/settings/config.cfg.example ~/settings/config.cfg`)
3. Open the configuration file with your favorite text editor (I prefer `nano`)
4. Update the configuration as follows:

####WiFi Config:
`wwwPath = /var/www/html`
`wifiHost = ESPXXXXXXX.local`
`wifiPort = 23`

Note - Replace ESPXXXXXX with the name of the access point you wrote down above.

####Serial Config
`wwwPath = /var/www/html`
`port = /dev/cu.USBXXXXXXX`

Note - Replace USBXXXXXXX with the name of the USB device you used when
flashing the firmware.

### Next Steps
At this point you have a working BrewPi installation, a working BrewPi script,
and firmware running on your ESP8266 board which can speak to both. The next
step from here is hooking up hardware and giving everything a try. Good luck,
and enjoy!


## Post-Brewpi Installation