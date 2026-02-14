# Selecting a Board

The BrewPi-ESP project supports a number of different microcontrollers with differing features. Supported controllers include:

| **Microcontroller**    | **DS18b20 Temp Sensors** | **Connected Relays** | **WiFi Support**   | **LCD2004 (IIC) Support** | **TFT Screen Support** | **Kasa WiFi Relay** | **Tilt/Inkbird Bluetooth Temp Sensors** |
|------------------------|--------------------------|----------------------|--------------------|---------------------------|------------------------|---------------------|-----------------------------------------|
| Arduino*               | :heavy_check_mark:       | :heavy_check_mark:   |                    | :heavy_check_mark:*       |                        |                     |                                         |
| ESP8266 _(deprecated)_ | :heavy_check_mark:       | :heavy_check_mark:   | :heavy_check_mark: | :heavy_check_mark:        |                        |                     |                                         |
| ESP32                  | :heavy_check_mark:       | :heavy_check_mark:   | :heavy_check_mark: | :heavy_check_mark:        | :heavy_check_mark:     | :heavy_check_mark:  | :heavy_check_mark:                      |
| ESP32-S2               | :heavy_check_mark:       | :heavy_check_mark:   | :heavy_check_mark: | :heavy_check_mark:        |                        | :heavy_check_mark:  |                                         |

**Note** - "Classic" BrewPi builds are based on Arduinos, which are shown in the table above for comparison. There is an (optional) IIC-compatible build of the Arduino firmware available, but "classic" builds did not support this feature.

:::{warning}
ESP8266 support is deprecated. Existing ESP8266 builds will continue to receive bug fixes for a limited time, but new features will only target the ESP32 and ESP32-S2. If you have an existing ESP8266 build using the LoLin D1 Mini, the LoLin S2 Mini (ESP32-S2) is a pin-compatible drop-in replacement.
:::

For new builds, I recommend the ESP32 as it supports the widest range of features. The specific ESP32 board I recommend is the [LoLin D32 Pro](https://www.aliexpress.us/item/2251832696801305.html) alongside their [TFT Screen](https://www.aliexpress.us/item/2251832733414978.html) and [TFT Cable](https://www.aliexpress.us/item/2251832662518722.html).

For smaller builds or builds using existing PCBs designed for the ESP8266, the [LoLin S2 Mini](https://www.aliexpress.us/item/3256802958877264.html) is recommended. This is a pin-compatible replacement for the LoLin D1 Mini and can be dropped into most existing builds.

I maintain a separate repo that contains all of my hardware designs on [GitHub](https://github.com/thorrak/thorrak_hardware/blob/master/BrewPi-ESP8266.md) with full BoMs and build instructions, as well as 3D-printable case designs. For a full how-to on building, take a look there. 
