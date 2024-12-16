#ifndef BREWPI_BTSCANNER_H
#define BREWPI_BTSCANNER_H

#ifdef HAS_BLUETOOTH

#include "Inkbird.h"
#include "Tilt.h"
#include <list>

#define BLE_SCAN_TIME 60*1000        // Milliseconds to scan (0=continuous scanning)
#define SCAN_FAIL_THRESHHOLD    (2*60*1000*1000)  // If we don't detect anything in 2 minutes, then the scanner failed. 

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

    bool scanning_failed();

    bool shouldRun;

    uint64_t last_detected_device_at;


private:
    uint64_t m_last_inkbird_purge_at;
    uint64_t m_last_tilt_purge_at;

};


extern btScanner bt_scanner;
extern std::list<inkbird> lInkbirds;
extern std::list<tilt> lTilts;


#endif // HAS_BLUETOOTH

#endif //BREWPI_BTSCANNER_H
