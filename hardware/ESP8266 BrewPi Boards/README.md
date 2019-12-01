ESP8266 BrewPi Controller Boards
================================

Although the "heart" of a BrewPi controller is the microprocessor running the BrewPi-compatible firmware, without other connected components that microcontroller cannot sense temperature, control power, or do any of the other things that make a temperature controller useful. Several PCBs have been designed to help simplify the wiring of the microcontroller to those other components.

Five different versions of the main PCB are listed here.  Each provide
different functionality and/or form factors:

*   No LCD Support, no buzzer, with screw connectors, and through-hole components
*   LCD Support with DuPont connectors, and surface-mount level converter components
*   LCD Support with DuPont connectors, through-hole components, and a SparkFun-based level converter sub-board
*   LCD Support with screw connectors, through-hole components, and a SparkFun-based level converter sub-board
*   LCD Support with DuPont connectors, through-hole components, and an integrated level converter

You may either purchase the boards using the links provided through pcbs.io, or you may download and use the Eagle files provided with a board supplier of your choice.  If you use the pcbs.io links, Thorrak will receive a small credit on the site.

The BOM links provided are intended to serve as examples of the parts needed and are not the only place to source the parts listed.  Generally speaking, AliExpress is less expensive for parts.  The trade-off is that sometimes you are buying 100 at a time, and in most cases you are waiting for shipping.  Mouser or other project part suppliers will probably have these parts as well.



| Board Design                                                                             | Order Link                           | Through Hole       | Surface Mount      | I2C LCD            | Connector Type  | Integrated Level Shifter |
|------------------------------------------------------------------------------------------|--------------------------------------|--------------------|--------------------|--------------------|-----------------|--------------------------|
| Surface Mount                                                                            | [Order](https://PCBs.io/share/8DDk0) |                    | :heavy_check_mark: | :heavy_check_mark: | Dupont          | :heavy_check_mark:       |
| [TH - Sparkfun /w Dupont](D1%20-%20LCD%20TH%20Dupont.md)                                 | [Order](https://PCBs.io/share/40D1X) | :heavy_check_mark: |                    | :heavy_check_mark: | Dupont          |                          |
| [TH - MOSFETs](https://github.com/brewpi-remix/brewpi-pcb-rmx/tree/master/D1%20Breakout) | [Order](https://pcbs.io/share/z5JLZ) | :heavy_check_mark: |                    | :heavy_check_mark: | Dupont          | :heavy_check_mark:       |
| [TH - Sparkfun /w Screw Terminals](D1%20-%20LCD%20TH%20Screws.md)                        | [Order](https://PCBs.io/share/4qpVq) | :heavy_check_mark: |                    | :heavy_check_mark: | Screw Terminals |                          |
| [TH - No LCD](D1%20-%20No%20LCD.md)                                                      | [Order](https://PCBs.io/share/49yVo) | :heavy_check_mark: |                    |                    | Screw Terminals |                          |




Features Explained
------------------

### Through Hole vs. Surface Mount

Through Hole components generally have a wire or pin that goes through a hole on the PCB, enabling easy soldering on the back-side of the circuit board. By contrast, Surface Mount components are attached directly to the surface of a PCB with small dots of solder. The overwhelming majority of PCBs use primarily surface mount components - but as these are more difficult to solder by hand, many hobbyists prefer hand-soldering through hole components. 

Choose "surface mount" if you want a compact board and are comfortable with learning to solder surface mount components. Choose "through hole" if you are not comfortable soldering and would prefer the easiest option.


### I2C LCD

The ESP8266 port of the BrewPi firmware supports the utilization of a LCD screen connected via an "I2C" interface. Due to the nature of these LCD screens, this requires additional hardware which increases the size & complexity of the board - but the slight amount of additional pain when building the controller pays off in the increased utility of having an LCD display.

Choose this option if you want an LCD screen. Ignore this option if you prefer the cheapest board available (and are OK with not having an LCD display).


### Connector Type - Dupont vs. Screw Terminals

"Dupont" cables are a type of cable that has a single female header at each end. This allows for connecting two devices with "pin header" interfaces through the use of a compact "ribbon"-type cable. 

Boards that are "dupont" style have compact pin-headers on them, and can only be used with Dupont cables. By contrast, "screw terminal" style boards are designed to have screw terminals soldered on which makes attaching/detaching cables easier. Screw-terminal boards are larger but can be optionally used with pin headers as well, however "dupont" style boards are not compatible with screw terminals.

Choose "dupont" if you want a cheaper, smaller board. Choose "screw terminals" if you prefer much larger soldering pads. 


### Integrated Level Shifter

I2C LCDs require that the 3.3 volt logic from the ESP8266 be "translated" to the 5 volt logic required by the LCD. This is done through the use of a "level shifter". Level shifters are relatively simple components, but require soldering a number of things by hand which some hobbyists would prefer not to do. The boards with integrated level shifters have these additional components as part of the PCB, while those boards without an integrated level shifter use a "sparkfun"-style level shifter which is attached to the board using pin headers. 


Choose this option if you want a smaller, more compact board.



