# Selecting a Board

There are a number of available ESP8266 modules out there, and an even greater
number of boards which feature them. Below are some of the boards which I have
tried and had success with (and a handful of boards which I wouldn't
recommend).

## Recommended Boards

Generally, I recommend that you use boards which contain a 4MB flash chip,
USB to UART bridge, and voltage regulation (allowing them to be powered off
+5v). Although we won't be using the NodeMCU firmware, NodeMCU boards hit this
sweet spot. I've linked example prices below - these may have changed since
writing.

### Wemos D1 Mini
This is the board which I targeted my installation for. Dirt cheap, tiny, a
great form factor, and with just enough GPIO pins - I love this thing.

Retailers:
* [Amazon](https://www.amazon.com/Winson-eseller-D1-mini-V2-development/dp/B01GFAO6VW) - $18.99 (Prime)
* [AliExpress](http://www.aliexpress.com/w/wholesale-wemos-d1-mini.html?site=glo&SearchText=wemos+d1+mini&g=y&SortType=price_asc&groupsort=1&shipCountry=us) - $2.60 (Shipped from China)
* [eBay](http://www.ebay.com/sch/i.html?_from=R40&_sacat=0&_nkw=wemos%20d1%20mini&rt=nc&LH_PrefLoc=1) - $9.69 (US Seller)


### NodeMCU v2
This is the board I initially developed this firmware with. It's larger than
the Wemos board, but has extra pins broken out. As of the time of writing, I've
also tested flashing this board with a Raspberry Pi. It works perfectly, and
doesn't require drivers.

Retailers:
* [Amazon](https://www.amazon.com/HiLetgo-Version-NodeMCU-Internet-Development/dp/B010O1G1ES) - $8.79 (Prime)
* [AliExpress](http://www.aliexpress.com/w/wholesale-nodemcu-v2.html?site=glo&SearchText=nodemcu+v2&g=y&SortType=price_asc&groupsort=1&shipCountry=us) - $2.92 (Shipped from China)
* [eBay](http://www.ebay.com/sch/i.html?_from=R40&_sacat=0&_sop=15&_nkw=nodemcu%20v2&rt=nc&LH_PrefLoc=1) - $9.90 (US Seller)


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