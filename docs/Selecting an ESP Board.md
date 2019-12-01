# Selecting a Board


When this project was originally released, there were a number of ESP8266 boards out there, and it wasn't clear which of
them would ultimately emerge from the pack as a de facto standard. Since then, one board has clearly risen from amongst
the others to be the I recommend exclusively for this project.

## Recommended Board

Generally, I recommend that you use boards which contain a 4MB flash chip,
USB to UART bridge, and voltage regulation (allowing them to be powered off
+5v). Although we won't be using the NodeMCU firmware, NodeMCU boards hit this
sweet spot. 

**Note** - While I've linked example prices below - these may have changed since
writing, and the specific boards linked are not guaranteed to work with this
project. Be careful who you buy from - there are unscrupulous manufacturers
out there who get cheap and don't test their boards (or even include all the
hardware they claim to!)

### LOLIN D1 Mini
This is the board which I targeted my installation for. Dirt cheap, tiny, a
great form factor, and with just enough GPIO pins - I love this thing. The manufacturer also seems to produce much more
reliable boards than their generic competitors. If you need a board, get this one.

Retailers:
* [LOLIN (AliExpress)](https://www.aliexpress.com/item/32529101036.html) - $4.55 (Shipped from China)

For those who have been following this project for awhile, this board is the same as the Wemos D1 Mini. Wemos renamed to
LOLIN which is the manufacturer linked above.

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
bridges, and voltage regulation. They also tend to be plentiful.