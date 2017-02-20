# Installing brewpi-esp8266

There are two main methods for installing brewpi-esp8266:
* Semi-automated alongside BrewPi using custom brewpi-tools
* Manual install after installing BrewPi

The below instructions explain the automated method. If you would prefer the
manual instructions, scroll to the bottom.


## Installation Instructions
1. Prepare the Raspberry Pi (install Raspbian, run raspi-config, update/upgrade)
2. Install BrewPi using the custom brewpi-tools.
3. Flash the firmware to the ESP8266 board.
4. Set up the firmware
5. Configure BrewPi to use your firmware


### 1. Prepare the Raspberry Pi - [[Video]](https://youtu.be/TdSnJOUgS3k)
Prior to installing BrewPi, you need to install Raspbian. Click the link above
to see how to prepare the Raspberry Pi using a Mac, or read the linked
instructions below for your operating system.

1. Download the latest version of Raspbian from [here](https://www.raspberrypi.org/downloads/raspbian/). I recommend the Lite version as I prefer headless installations, but the full version works as well.
2. Burn Raspbian to your SD card using [these instructions](https://www.raspberrypi.org/documentation/installation/installing-images/).
3. *NEW* - [Enable SSH](https://www.raspberrypi.org/documentation/remote-access/ssh/) on your Raspberry Pi by writing an empty file named "ssh" to the root of the SD card.
4. *Optional, NEW* - Configure WiFi - See the note below if you want to configure WiFi now, thus preventing having to find an ethernet cable
5. Plug the SD card into your Raspberry Pi, connect the Pi to ethernet (if you did not configure WiFi), and plug in power.
6. Locate the IP address for your Raspberry Pi This can generally be done by executing `arp -a | grep raspberry` however you can also locate your Raspberry Pi by logging into your router and looking for the device.
7. Update the Raspberry Pi software by running `sudo apt-get update` and `sudo apt-get upgrade`.
8. Run `raspi-config` and configure the Pi. At a minimum, expand the filesystem (option 1).
9. *Optional* - [Configure WiFi](https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md) on your Raspberry Pi (if needed)


### 2. Install BrewPi using custom brewpi-tools - [[Video]](http://www.youtube.com/watch?v=vUaPao_wBGI)

1. Log into your BrewPi and download the custom brewpi tools using this command: `git clone https://github.com/thorrak/brewpi-tools`.
2. Execute `sudo brewpi-tools/install.sh`
3. Execute `sudo brewpi-tools/install-esp8266.sh`


### 3. Flash the firmware to the ESP8266 board. - [[Video]](http://www.youtube.com/watch?v=vUaPao_wBGI)
As there are a number of ways to do this (depending on your operating system,
the board you are using, etc.) these instructions are located in a
[different file](Installing the Firmware.md). Go read them, and come back.


### 4. Set up the firmware - [[Video]](http://www.youtube.com/watch?v=xtkuAVaX8JQ)
Once you have flashed the firmware to the ESP8266 board, you will need to
configure it for use. If you are using the "serial" version of the firmware
then congratulations - you're done! For the WiFi version, you will need to
configure access to your wireless network.

**Note:  You will lose connectivity to the Internet following these instructions. Ensure you are either on a second device, or you have these instructions printed out.**

1.	From a wifi-enabled device (computer, phone, etc.) look for a new access point named "ESPXXXXXXX" where XXXXX is a number.
2.	**Write down the access point name (ESPXXXXXX).  You will need this later.**
3.	Connect to this access point and open a web browser to any page.  Some devices may automatically open a web browser.  
4.	The ESP8266 will display its web page.
5.	Click "Configure WiFi"
6.	Select your local wireless network & enter your password
7.	Click "Save"

At this point you may re-connect to your regular Wi-Fi.


### 5. Configure BrewPi to use your firmware - [[Video]](http://www.youtube.com/watch?v=xtkuAVaX8JQ)
Now that you've set up the firmware you need to point the BrewPi script at it.

1. Copy the default configuration file: `sudo cp /home/brewpi/settings/config.cfg.example /home/brewpi/settings/config.cfg`
2. Open the configuration file with your favorite text editor (I prefer `nano`):  `sudo nano /home/brewpi/settings/config.cfg`
3. Update the configuration as follows (Note - You will only need one of these - WiFi for the WiFi firmware, Serial for the serial firmware):

####WiFi Config:
`wwwPath = /var/www/html`  
`wifiHost = ESPXXXXXXX.local`  
`wifiPort = 23`

**Note - Replace ESPXXXXXX with the name of the access point you wrote down above.**

... or ...

####Serial Config
`wwwPath = /var/www/html`  
`port = /dev/cu.USBXXXXXXX`

**Note - Replace `cu.USBXXXXXXX` with the name of the USB device you used when
flashing the firmware. (Probably cu.USBXXXX or ttyUSBX)**

### Next Steps
At this point you have a working BrewPi installation, a working BrewPi script,
and firmware running on your ESP8266 board which can speak to both. The next
step from here is hooking up hardware and giving everything a try. Good luck,
and enjoy!


## Manual Installation
If you have already installed BrewPi but want to use the ESP8266 firmware,
you will need to do the following:

1. Replace the default brewpi-script with the custom brewpi-script that will work with ESP8266 devices (Located [here](https://github.com/thorrak/brewpi-script))
2. Clone the brewpi-esp8266 firmware (this repo) into `~brewpi/`
3. Install esptool `sudo pip install esptool --upgrade`
4. Follow the instructions above beginning with step 3.


### Raspberry Pi Setup - Additional Info about Install-Time WiFi Configuration
In early 2016 there was [an update](https://www.raspberrypi.org/blog/another-update-raspbian/) made to Raspbian which allows for install-time configuration of WiFi. Buried deep within a blog post, there was the following note:

"If a wpa_supplicant.conf file is placed into the /boot/ directory, this will be moved to the /etc/wpa_supplicant/ directory the next time the system is booted, overwriting the network settings; this allows a Wifi configuration to be preloaded onto a card from a Windows or other machine that can only see the boot partition."

While not discussed in the official documentation, this greatly simplifies headless configuration (especially for Raspberry Pi zeros).

To utilize this, prior to the initial boot on a newly flashed Raspbian installation, create a `wpa_supplicant.conf` file in the `/boot` directory of the SD card with the following contents (adjusting to match your network configuration):

`network={
    ssid="YOUR_SSID"
    psk="YOUR_PASSWORD"
    key_mgmt=WPA-PSK
}`