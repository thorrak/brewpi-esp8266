# Selecting a Board

There are a number of available ESP8266 modules out there, and an even greater
number of boards which feature them. Below are some of the boards which I have
tried and had success with (and a handful of boards which I wouldn't
recommend).

## Recommended Boards

Generally, I recommend that you use boards which contain a 4MB flash chip,
USB to UART bridge, and voltage regulation (allowing them to be powered off
+5v). Although we won't be using the NodeMCU firmware, NodeMCU boards hit this
sweet spot. 

**Note** - While I've linked example prices below - these may have changed since
writing, and the specific boards linked are not guaranteed to work with this
project. Be careful who you buy from - there are unscrupulous manufacturers
out there who get cheap and don't test their boards (or even include all the
hardware they claim to!)

### Wemos D1 Mini
This is the board which I targeted my installation for. Dirt cheap, tiny, a
great form factor, and with just enough GPIO pins - I love this thing. If I
was recommending a board, this would be it. 

Retailers:
* [WeMos (AliExpress)](https://wiki.wemos.cc/products:d1:d1_mini) - $4.96 (Shipped from China)
* [Amazon](https://www.amazon.com/Makerfocus-NodeMcu-Development-ESP8266-Compatible/dp/B01N3P763C/) - $8.99 (Prime)
* [AliExpress](https://www.aliexpress.com/item/Smart-Electronics-D1-mini-Mini-NodeMcu-4M-bytes-Lua-WIFI-Internet-of-Things-development-board-based/32651255381.html) - $2.60 (Shipped from China)
* [eBay](https://www.ebay.com/itm/D1-Mini-NodeMCU-and-Arduino-compatible-wifi-lua-ESP8266-ESP-12-Arduino-WeMos/201714015357) - $6.49 (US Seller)


### NodeMCU v2
This is the board I initially developed this firmware with. It's larger than
the Wemos board but has extra pins broken out.

Retailers:
* [Amazon](https://www.amazon.com/HiLetgo-Internet-Development-Wireless-Micropython/dp/B010O1G1ES/) - $8.39 (Prime)
* [AliExpress](https://www.aliexpress.com/item/Update-Industry-4-0-New-esp8266-NodeMCU-v2-Lua-WIFI-networking-development-kit-board-based-on/32358722888.html) - $2.98 (Shipped from China)
* [eBay](https://www.ebay.com/itm/ESP8266-microcontroller-NodeMCU-Lua-WIFI-with-CP2102-USB/322730954299) - $8.57 (US Seller)


## Not Recommended

### NodeMCU "v3"/LoLin
Why is this board "v3"? Probably an attempt by overseas sellers to differentiate
their boards from their competitors. Honestly, the board itself is probably fine,
but it takes up the full width of my breadboard - not leaving any pins available
for hooking up hardware. If you want this form factor, go with a v2 board and
you'll have that much more room to tinker.

### ESP-01
This thing is the granddaddy of the popular ESP boards. Originally purposed for use as
a serial-to-WiFi bridge for things such as the Arduino, this is probably the
most common board in the ESP8266 family, and is certainly the one most people have
familiarity with.

Unfortunately, this thing also only has 2 GPIO pins, 512kb of flash, no UART
bridge, and no voltage regulators. Could you get this firmware working on it?
Actually, yes - you probably could. Would I recommend it? Given the price for
better boards - not unless you had absolutely no alternative.



## NodeMCU vs. ESP8266
Although this firmware targets the ESP8266, there are multiple versions of this
controller as well as several common boards that contain it.

When you see
"ESP8266" referenced, generally what is being discussed is a module combining a
WiFi antenna, Tensilica processor, and flash memory. A list of available modules
can be found on [Wikipedia](https://en.wikipedia.org/wiki/ESP8266). Although
this firmware can probably be used on most of these, I highly recommend only
using boards built with one of the ESP-12F modules due to the increased flash
size. Additionally, features such as a USB to UART chip and voltage regulators
greatly simplify the installation

Thankfully, there is a group of boards which feature just about all the
features we could want -- and are dirt cheap to boot. Although NodeMCU is
actually an alternative firmware for the ESP8266 featuring Lua support, boards
which are designed for the firmware generally come with ESP-12F modules, UART
bridges, and voltage regulation. They also tend to be plentiful