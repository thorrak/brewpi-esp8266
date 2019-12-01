ESP8266 - Through Hole w/o LCD
==================================================

This PCB is designed to allow connecting an ESP8266 to the other hardware necessary to run the BrewPi firmware and control fermentation temperatures for your brewery. This specific board was designed to use a LoLin D1 Mini ESP8266 microcontroller and integrate with a 2-channel relay board. It does NOT support an LCD screen. This PCB supports the use of dupont connectors for connecting the relay which allows for a much more compact design than if screw terminals were used.

This board does not have terminals for directly connecting temperature sensors - it is intended to have an RJ-45 jack soldered on, and then be connected to a separate RJ-45 sensor board via an ethernet cable. 


Order Links
-----------

[Order from PCBs.io](https://PCBs.io/share/49yVo):

- 2 Layer Board - 2.5554 sq in (1.7829in x 1.4333in) / 1648.64 sq/mm (45.29mm x 36.41mm)
- $9.94 per set of 4 ($3.89 per sq in)

| Top View          | Bottom View          |
| ----------------- |:--------------------:|
| ![Board Top][top] | ![Board Bottom][bot] |

[top]: imgs/No%20LCD%20Top.png "Board Top"
[bot]: imgs/No%20LCD%20Bottom.png "Board Bottom"


Bill of Materials (Incomplete)
------------------------------

| Part                                                                                              | Qty | Build Cost | Order Qty | Order Cost* |
|---------------------------------------------------------------------------------------------------|-----|------------|-----------|-------------|
| [PCB](https://PCBs.io/share/49yVo)                                                                | 1   | $2.49      | 4         | $9.94       |
| [RJ45 Modular Jack w/no shield](https://www.aliexpress.com/item/32736146888.html) (J1)            | 1   | $0.13      | 20        | $2.52       |
| [Wago 2-terminal Screw Clamp](https://www.aliexpress.com/item/32700056337.html) (X1)              | 1   | $0.02      | 100       | $1.95       |

* Order cost is the cost to order the linked item, in its minimum quantity, excluding shipping.

- Total Build Cost (Individual): $1.95
- Total Build Cost (Min Order): $11.68 (Builds 4)



Old bill of materials (incomplete)
----------------------------------

1x [2-Pin 5mm Pitch Screw Terminal](https://www.aliexpress.com/item/100PCS-2-Pin-Screw-Terminal-Block-Connector-5mm-Pitch-Free-Shipping/32700056337.html)
1x 4-Pin Pin Header
1x 4.7k 1/4 Watt Resistor
1x RJ-45 Jack
1x [LOLIN D1 Mini ESP8266 board](https://wiki.wemos.cc/products:d1:d1_mini)





Build Notes
-----------

This board is intended to be used with an additional "sensor breakout" PCB to which the DS18b20 temperature sensors will need to be connected. 


