# BrewPi ESP8266 PCB Information

In general, building an ESP8266-based BrewPi is greatly simplified by using a
PCB which does most of the wiring for you. For the purposes of this document,
we're splitting the build into two parts - the "Main PCB" to which the
ESP8266 is connected, and the "Sensor Breakout" which connects to the
temperature and (optionally) door sensor.

## Main PCB Overview

The "main PCB" connects the ESP8266 and provides breakouts for the relay board,
an I2C LCD, and some means of connecting to either the sensors themselves or
to the "sensor breakout" discussed below. 

Four different versions of the main PCB are listed here.  Each provide 
different functionality and/or form factors:

*   No LCD Support, no buzzer, with screw connectors, and through-hole components
*   LCD Support with DuPont connectors, and surface-mount level converter components
*   LCD Support with DuPont connectors, through-hole components, and a SparkFun-based level converter sub-board
*   LCD Support with screw connectors, through-hole components, and a SparkFun-based level converter sub-board

You may either purchase the boards using the links provided through pcbs.io, or you may download and use the Eagle files provided with a board supplier of your choice.  If you use the pcbs.io links, Thorrak will receive a small credit on the site. 

The BOM links provided are intended to serve as examples of the parts needed and are not the only place to source the parts listed.  Generally speaking, AliExpress is less expensive for parts.  The trade-off is that sometimes you are buying 100 at a time, and in most cases you are waiting for shipping.  Mouser or other project part suppliers will probably have these parts as well.

#### Choosing a PCB

Generally, smaller PCBs are cheaper, and so the main trade-off in each of the
designs is space vs. functionality/ease of use/ease to build.

The smallest/cheapest board available has no LCD support, uses screw connectors for 
the relay, RJ-11 for connecting sensors, and uses a through-hole resistor
(as opposed to surface mount components which can be harder to solder).

The next smallest board uses primarily surface mount components. These can be
hard to solder, but result in a very small, clean, and professional looking
board. These also allow us to do the level-shifting necessary to run the LCD
without requiring an external level-shifting board. While this is the board
I prefer the use of surface mount components can be challenging to solder
hence why many prefer the through-hole boards. This board comes in a RJ-11
and RJ-45 variant - see the "sensor breakout" discussion below for more
info.

The third smallest PCB replaces the surface mount components with through-hole
components, simplifying the soldering of the board. As a result, however, it
requires a separate level-shifter board to allow the LCD to function. Currently,
this board only comes in an RJ-11 variant.

The largest PCB replaces the "DuPont" style connectors of the previous PCB
with screw terminals. Some people find screw terminals easier to use as
they tend to be larger, but in my opinion this is a matter of personal
preference. Currently, this board only comes in an RJ-11 variant.
 
Now that you can make an informed decision, let's look at the PCBs themselves:


### No LCD Support, no buzzer, with screw connectors, and through-hole components.

[Purchase Boards](https://pcbs.io/share/zMGb8)  

Eagle Files:

*   Coming Soon

##### Bill of Materials:

1x [2-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100PCS-2-Pin-Screw-Terminal-Block-Connector-5mm-Pitch-Free-Shipping/32700056337.html)  
1x [4-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100-pcs-4-Pin-Screw-blue-PCB-Terminal-Block-Connector-5mm-Pitch/32658656423.html) (Alternatively, 4x 2-pin terminals)  
1x [10k 1/4 Watt Axial Resistor](https://www.aliexpress.com/item/100pcs-10k-ohm-1-4W-10k-Metal-Film-Resistor-10kohm-0-25W-1-ROHS/32577051768.html)  
1x [RJ-11 Jack](https://www.aliexpress.com/item/RJ11-socket-outlet-90-degree-6-core-crystal-head-95001-6P6C-black-base-HXDD2/32685158354.html)  
1x [WeMos D1 Mini ESP8266 board](https://wiki.wemos.cc/products:d1:d1_mini)



### LCD Support with DuPont connectors, and surface-mount level converter components.

[Purchase Boards - RJ11 Variant](https://PCBs.io/share/zy1q2)  
[Purchase Boards - RJ45 Variant](https://PCBs.io/share/8DDk0)  

Eagle Files:

RJ-11 Variant:
*   [D1 Breakout - LCD SMD Dupont.brd](https://github.com/thorrak/brewpi-esp8266/blob/master/hardware/D1%20Breakout%20-%20LCD%20SMD%20Dupont.brd)
*   [D1 Breakout - LCD SMD Dupont.sch](https://github.com/thorrak/brewpi-esp8266/blob/master/hardware/D1%20Breakout%20-%20LCD%20SMD%20Dupont.sch)

RJ-45 Variant:

*   (Coming soon)

##### Bill of Materials:

1x [2-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100PCS-2-Pin-Screw-Terminal-Block-Connector-5mm-Pitch-Free-Shipping/32700056337.html)  
1x [2-Pin Pin Header](https://www.aliexpress.com/item/Free-Shipping-40-Pins-Header-Strip-Socket-2-54mm-Straight-Single-Row-Pin-Male-For-Arduino/2046893919.html)  
2x [4-Pin Pin Header](https://www.aliexpress.com/item/Free-Shipping-40-Pins-Header-Strip-Socket-2-54mm-Straight-Single-Row-Pin-Male-For-Arduino/2046893919.html)  
5x [10k 0805 Resistor](https://www.aliexpress.com/item/Free-shipping-500pcs-RES-ORIGINAL-SMD-Resistor-1-0805-10k-10K-chip-resistor-1-8W-Good/32270620277.html)  
1x [100uF 1206 Capacitor](https://www.aliexpress.com/item/Free-shipping-1206-SMD-capacitor-100uf-16V-107K-100PCS/32375666957.html) (Optional)  
2x [BSS138 MOSFET](https://www.aliexpress.com/item/50PCS-BSS138LT1G-SOT23-BSS138-SOT-MOSFET-SMD-new-and-original-IC-free-shippin/32518915182.html)  
1x [RJ-11 Jack](https://www.aliexpress.com/item/RJ11-socket-outlet-90-degree-6-core-crystal-head-95001-6P6C-black-base-HXDD2/32685158354.html)  (RJ-11 variant only)  
1x [RJ-45 Jack](https://www.aliexpress.com/item/High-Quality-20pcs-RJ45-8P8C-Computer-Internet-Network-PCB-Jack-Socket-Black/32736146888.html) (RJ-45 variant only)  
1x [WeMos D1 Mini ESP8266 board](https://wiki.wemos.cc/products:d1:d1_mini)  
1x [LCD 20x4 I2C LCD Screen](https://www.aliexpress.com/item/Free-shipping-LCD-module-Blue-screen-IIC-I2C-2004-5V-20X4-LCD-board-provides-library-files/1873368596.html)

### LCD Support with DuPont connectors, through-hole components, and a SparkFun-based level converter sub-board.

[Purchase Boards](https://pcbs.io/share/46AL1)   

Eagle Files:

*   [D1 Breakout - LCD TH Dupont.brd](https://github.com/thorrak/brewpi-esp8266/blob/master/hardware/D1%20Breakout%20-%20LCD%20TH%20Dupont.brd)
*   [D1 Breakout - LCD TH Dupont.sch](https://github.com/thorrak/brewpi-esp8266/blob/master/hardware/D1%20Breakout%20-%20LCD%20TH%20Dupont.sch)

##### Bill of Materials:

1x [2-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100PCS-2-Pin-Screw-Terminal-Block-Connector-5mm-Pitch-Free-Shipping/32700056337.html)  
1x [2-Pin Pin Header](https://www.aliexpress.com/item/Free-Shipping-40-Pins-Header-Strip-Socket-2-54mm-Straight-Single-Row-Pin-Male-For-Arduino/2046893919.html)  
2x [4-Pin Pin Header](https://www.aliexpress.com/item/Free-Shipping-40-Pins-Header-Strip-Socket-2-54mm-Straight-Single-Row-Pin-Male-For-Arduino/2046893919.html)  
1x [10k 1/4 Watt Axial Resistor](https://www.aliexpress.com/item/100pcs-10k-ohm-1-4W-10k-Metal-Film-Resistor-10kohm-0-25W-1-ROHS/32577051768.html)  
1x [Sparkfun (or compatible) Level Shifter](https://www.aliexpress.com/item/1pcs-Logic-Level-Shifter-Bi-Directional-For-Arduino-Four-Way-Ttwo-Way-Logic-Level-Transformation-Module/32624272876.html)  
1x [RJ-11 Jack](https://www.aliexpress.com/item/RJ11-socket-outlet-90-degree-6-core-crystal-head-95001-6P6C-black-base-HXDD2/32685158354.html)  
1x [WeMos D1 Mini ESP8266 board](https://wiki.wemos.cc/products:d1:d1_mini)  
1x [LCD 20x4 I2C LCD Screen](https://www.aliexpress.com/item/Free-shipping-LCD-module-Blue-screen-IIC-I2C-2004-5V-20X4-LCD-board-provides-library-files/1873368596.html)

### LCD Support with screw connectors, through-hole components, and a SparkFun-based level converter sub-board.

[Purchase Boards](https://pcbs.io/share/46AR1)  

Eagle Files:

*   [D1 Breakout - LCD TH Screws.brd](https://github.com/thorrak/brewpi-esp8266/blob/master/hardware/D1%20Breakout%20-%20LCD%20TH%20Screws.brd)
*   [D1 Breakout - LCD TH Screws.sch](https://github.com/thorrak/brewpi-esp8266/blob/master/hardware/D1%20Breakout%20-%20LCD%20TH%20Screws.sch)

##### Bill of Materials:

1x [2-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100PCS-2-Pin-Screw-Terminal-Block-Connector-5mm-Pitch-Free-Shipping/32700056337.html)  
2x [4-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100-pcs-4-Pin-Screw-blue-PCB-Terminal-Block-Connector-5mm-Pitch/32658656423.html) (Alternatively, 4x 2-pin terminals)  
1x [2-Pin Pin Header](https://www.aliexpress.com/item/Free-Shipping-40-Pins-Header-Strip-Socket-2-54mm-Straight-Single-Row-Pin-Male-For-Arduino/2046893919.html)  
1x [10k 1/4 Watt Axial Resistor](https://www.aliexpress.com/item/100pcs-10k-ohm-1-4W-10k-Metal-Film-Resistor-10kohm-0-25W-1-ROHS/32577051768.html)  
1x [Sparkfun (or compatible) Level Shifter](https://www.aliexpress.com/item/1pcs-Logic-Level-Shifter-Bi-Directional-For-Arduino-Four-Way-Ttwo-Way-Logic-Level-Transformation-Module/32624272876.html)  
1x [RJ-11 Jack](https://www.aliexpress.com/item/RJ11-socket-outlet-90-degree-6-core-crystal-head-95001-6P6C-black-base-HXDD2/32685158354.html)  
1x [WeMos D1 Mini ESP8266 board](https://wiki.wemos.cc/products:d1:d1_mini)  
1x [LCD 20x4 I2C LCD Screen](https://www.aliexpress.com/item/Free-shipping-LCD-module-Blue-screen-IIC-I2C-2004-5V-20X4-LCD-board-provides-library-files/1873368596.html)
