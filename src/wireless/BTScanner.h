#ifndef BREWPI_BTSCANNER_H
#define BREWPI_BTSCANNER_H

#ifdef HAS_BLUETOOTH

#include "Inkbird.h"

#define BLE_SCAN_TIME 60        // Seconds to scan (0=continuous scanning)
// #define BT_SCANNER_INKBIRD_PURGE_TIME   (60)               // How long after an Inkbird was last detected before we tag it as "stale" and delete it (in seconds)

class btScanner
{
public:
    btScanner();
    void init();
    bool scan();
    void stop_scan();

    void load_kegtron_from_advert(NimBLEAdvertisedDevice* advertisedDevice);

    inkbird* get_or_create_inkbird(const NimBLEAddress devAddress);
    inkbird* get_inkbird(const NimBLEAddress devAddress);

    void purge_stale_inkbirds();

    bool shouldRun;

private:
    uint32_t m_last_inkbird_purge_at;

};


extern btScanner bt_scanner;
extern std::list<inkbird> lInkbirds;


#endif // HAS_BLUETOOTH

#endif //BREWPI_BTSCANNER_H
