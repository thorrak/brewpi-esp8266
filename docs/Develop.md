#Developing/Compiling brewpi-esp8266


##Introduction

So you want to help develop brewpi-esp8266? Great! There are plenty of features
which are not currently implemented, bugs left unsquished, optimizations left
unoptimized - the sky's the limit.


## Required Tools
You will need:
* [Arduino 1.6.9](https://www.arduino.cc/en/Main/Software) or later
* [ESP8266 support for Arduino](https://github.com/esp8266/Arduino)
* [Visual Studio 2015](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx) (Community Edition works perfectly)
* [Visual Micro](http://www.visualmicro.com/page/Arduino-Visual-Studio-Downloads.aspx) (can be installed within Visual Studio)

If you're installing from scratch, I highly recommend installing these in the
order listed above. **For Visual Studio 2015, be sure to install support for
C++.** Once you are done installing the toolchain, set up Visual Micro to point
to your Arduino installation and configure your ESP8266/NodeMCU board.


## Required Arduino Libraries
One key change from the original Arduino version of the firmware is that I
went back to using contributed libraries in favor of local copies when
possible. To compile this project you will need to make sure you have updated
copies of the following available in your Arduino environment:
* OneWire
* (TODO - Fill out this list, I think there are others)


## A note from your friendly developer
###Well, hello there!
If you are familiar at all with Visual Micro, Visual Studio,
or Atmel Studio you probably noticed that this project was assembled in fairly
haphazard fashion. This was admittedly my first project using Visual Micro (and
is perhaps my second or third using Visual Studio - and the first in half a
decade). Beyond just the code, if you have any ideas on how to use any of these
tools more effectively, please contribute!

###GCC/Xtensa Toolchain
Although I haven't tried it, during the cursory research I did prior to starting
this project I came across documentation suggesting that one could use the
GCC/Xtensa toolchain to compile the code into a flashable binary and eliminate
having to install Visual Studio/Visual Micro (and possibly parts of the
Arduino/ESP8266 support software as well). If you know how to get this going,
then awesome! Please submit a pull request!
