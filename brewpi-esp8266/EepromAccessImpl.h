#pragma once


#ifdef ESP8266
#include "ESPEepromAccess.h"
typedef ESPEepromAccess EepromAccess;
#else
#error "Other implementations than ESP8266 no longer supported!"
#endif
