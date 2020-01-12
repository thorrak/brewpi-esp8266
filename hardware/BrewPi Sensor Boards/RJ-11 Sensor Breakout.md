RJ-11 Sensor Breakout
=====================

**NOTE** - For new builds, the use of the RJ-11 boards are _not_ recommended. The RJ-45 boards are recommended instead.

This board is intended to allow for the DS18b20 temperature sensors (and optionally, door switch) to be connected to a PCB that is away from your main BrewPi controller, thereby allowing for the use of shorter temperature sensor cables - or for the placement of the sensor breakout inside your fermentation chamber if necessary. This breakout board is intended to be connected to the BrewPi controller via an RJ-11 straight-through cable.

**Be very careful when choosing a cable to connect your sensor board to your BrewPi controller - not all RJ-11 cables are straight through** 

Temperature sensors are intended to be soldered directly to the board, and the door sensor can either be soldered directly to the board or can be connected via an optional screw terminal.

Order Links
-----------

[Order from PCBs.io](https://PCBs.io/share/r16lD):

- 2 Layer Board - 1.8241 sq in (1.2370in x 1.4746in) / 1176.84 sq/mm (31.42mm x 37.45mm)
- $7.21 per set of 4

| Top View          | Bottom View          |
| ----------------- |:--------------------:|
| ![Board Top][top] | ![Board Bottom][bot] |

[top]: imgs/RJ-11%20Top.png "Board Top"
[bot]: imgs/RJ-11%20Bottom.png "Board Bottom"


Bill of Materials
-----------------

| Part                                                                                              | Qty | Build Cost | Order Qty | Order Cost* |
|---------------------------------------------------------------------------------------------------|-----|------------|-----------|-------------|
| [PCB](https://PCBs.io/share/r16lD)                                                                | 1   | $1.80      | 4         | $7.21       |
| [RJ11 Modular Jack w/no shield](https://www.aliexpress.com/item/32843453633.html) (J1)            | 1   | $0.33      | 10        | $3.33       |
| [Wago 2-terminal Screw Clamp](https://www.aliexpress.com/item/32700056337.html) (X1) (Optional)   | 1   | $0.02      | 100       | $1.95       |

* Order cost is the cost to order the linked item, in its minimum quantity, excluding shipping.

- Total Build Cost (Individual): $2.15
- Total Build Cost (Min Order): $12.49 (Builds 4)



Build Notes
-----------

The DS18b20 temperature sensors are intended to be soldered directly to the PCB. This helps keep them secure, but can make them difficult to remove. Testing your temperature sensors before attaching them is highly recommended.

The use of the door sensor is optional. If the door sensor is used, only the screw terminal OR direct solder points should be used. Attaching multiple door sensors will have unintended effects.

This board is designed to be used inside an optional case. A case design is available on [Thingiverse](https://www.thingiverse.com/thing:2954861).


