#ifndef BREWPI_BTSCANNER_H
#define BREWPI_BTSCANNER_H

#ifdef HAS_BLUETOOTH

#include "Inkbird.h"
#include "Tilt.h"

#define BLE_SCAN_TIME 60        // Seconds to scan (0=continuous scanning)

class btScanner
{
public:
    btScanner();
    void init();
    bool scan();
    void stop_scan();

    inkbird* get_or_create_inkbird(const NimBLEAddress devAddress);
    inkbird* get_inkbird(const NimBLEAddress devAddress);

    tilt* get_or_create_tilt(const NimBLEAddress devAddress);
    tilt* get_tilt(const NimBLEAddress devAddress);

    void purge_stale_inkbirds();
    void purge_stale_tilts();

    bool shouldRun;

private:
    uint32_t m_last_inkbird_purge_at;
    uint32_t m_last_tilt_purge_at;

};


extern btScanner bt_scanner;
extern std::list<inkbird> lInkbirds;
extern std::list<tilt> lTilts;


#endif // HAS_BLUETOOTH

#endif //BREWPI_BTSCANNER_H
