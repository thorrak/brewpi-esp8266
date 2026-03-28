#include <esp_mac.h>
#include <cstring>
#include "getGuid.h"

void getGuid(char *str)
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    uint64_t chipid = 0;
    for (int i = 0; i < 6; i++) chipid |= ((uint64_t)mac[i]) << (i * 8);
    uint32_t int32_1, int32_2;

    int32_1 = chipid & 0x00000000FFFFFFFF;
    int32_2 = (chipid & 0xFFFFFFFF00000000) >> 32;

    char first[9], secon[9];
    sprintf(first, "%08lX", (unsigned long)int32_1);
    sprintf(secon, "%08lX", (unsigned long)int32_2);

    strcpy(str, first);
    strcat(str, secon);
}
