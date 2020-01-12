ESP8266 - Through Hole /w LCD & Screw Terminal PCB
==================================================

This PCB is designed to allow connecting an ESP8266 to the other hardware necessary to run the BrewPi firmware and control fermentation temperatures for your brewery. This specific board was designed to use a LoLin D1 Mini ESP8266 microcontroller and integrate with a 2-channel relay board and a LCD2004 IIC LCD screen. This PCB supports the use of screw terminal connectors for connecting the relay and IIC LCD which some builders find to be easier to use due to their larger size. This PCB uses a "SparkFun"-style level shifter module for handling the level conversion necessary to drive the LCD display.

This board does not have terminals for directly connecting temperature sensors - it is intended to have an RJ-45 jack soldered on, and then be connected to a separate RJ-45 sensor board via an ethernet cable. 


Order Links
-----------

[Order from PCBs.io](https://PCBs.io/share/4qpVq):

- 2 Layer Board - 4.3815 sq in (1.8579in x 2.3583in) / 2826.77 sq/mm (47.19mm x 59.9mm)
- $16.82 per set of 4 ($3.84 per sq in)

| Top View          | Bottom View          |
| ----------------- |:--------------------:|
| ![Board Top][top] | ![Board Bottom][bot] |

[top]: imgs/LCD%20TH%20Screws%20Top.png "Board Top"
[bot]: imgs/LCD%20TH%20Screws%20Bottom.png "Board Bottom"


Bill of Materials (Incomplete)
------------------------------

| Part                                                                                              | Qty | Build Cost | Order Qty | Order Cost* |
|---------------------------------------------------------------------------------------------------|-----|------------|-----------|-------------|
| [PCB](https://PCBs.io/share/4qpVq)                                                                | 1   | $4.21      | 4         | $16.82      |
| [RJ45 Modular Jack w/no shield](https://www.aliexpress.com/item/32736146888.html) (J1)            | 1   | $0.13      | 20        | $2.52       |
| [Wago 2-terminal Screw Clamp](https://www.aliexpress.com/item/32700056337.html) (X1)              | 1   | $0.02      | 100       | $1.95       |
| [SparkFun Logic Level Converter](https://www.sparkfun.com/products/12009)                         | 1   | $2.51      | 1         | $2.51       |

* Order cost is the cost to order the linked item, in its minimum quantity, excluding shipping.

- Total Build Cost (Individual): $1.95
- Total Build Cost (Min Order): $11.68 (Builds 4)

There are sellers on AliExpress that sell "SparkFun"-style logic level converters -- If you order from one of these sellers, please be sure that the pinout is the same (HV on one side, LV on the other side, with GND and HV/LV in the middle).


Old bill of materials (incomplete)
----------------------------------

1x [2-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100PCS-2-Pin-Screw-Terminal-Block-Connector-5mm-Pitch-Free-Shipping/32700056337.html)
2x [4-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100-pcs-4-Pin-Screw-blue-PCB-Terminal-Block-Connector-5mm-Pitch/32658656423.html) (Alternatively, 4x 2-pin terminals)
1x 4.7k 1/4 Watt Resistor
1x [Sparkfun (or compatible) Level Shifter](https://www.aliexpress.com/item/1pcs-Logic-Level-Shifter-Bi-Directional-For-Arduino-Four-Way-Ttwo-Way-Logic-Level-Transformation-Module/32624272876.html)
1x RJ-45 Jack
1x [LOLIN D1 Mini ESP8266 board](https://wiki.wemos.cc/products:d1:d1_mini)



Build Notes
-----------

This board is intended to be used with an additional "sensor breakout" PCB to which the DS18b20 temperature sensors will need to be connected. 


