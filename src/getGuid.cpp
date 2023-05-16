#include <Arduino.h>
#include "getGuid.h"

void getGuid(char *str)
{
#if defined(ESP8266)
    strcpy(str, String(ESP.getChipId()).c_str());
#elif defined(ESP32)

    uint64_t chipid = ESP.getEfuseMac();
    uint32_t int32_1, int32_2;

    int32_1 = chipid & 0x00000000FFFFFFFF;
    int32_2 = (chipid & 0xFFFFFFFF00000000) >> 32;

    char first[9], secon[9];
    sprintf(first, "%08X", int32_1);
    sprintf(secon, "%08X", int32_2);

    strcpy(str, first);
    strcat(str, secon);
#else
#error "Invalid device selected!"
#endif
}
