#ifndef TPLINK_KASA_TPLINKSCANNER_H
#define TPLINK_KASA_TPLINKSCANNER_H

#include <list>
#include <IPAddress.h>
#include "TPLinkPlug.h"
#include "TPLinkConnector.h"


#define TPLINK_DISCOVER_EVERY           30  // How frequently to send discovery packets (in seconds)
#define TPLINK_SAFETY_TIMEOUT_REFRESH   20  // How frequently to refresh the "safety" off countdown (in seconds)
#define TPLINK_SAFETY_TIMEOUT           (TPLINK_SAFETY_TIMEOUT_REFRESH * 3 + 5)  // How long to set the "safety" off countdown to turn the device off after

class TPLinkScanner {

public:
    TPLinkConnector tplink_connector;

    // Device Lists
    std::list<TPLinkPlug> lTPLinkPlugs;

    TPLinkPlug* get_tplink_plug(const char *deviceMAC, const char *childID);
    TPLinkPlug* get_or_create_tplink_plug(IPAddress ip_addr, const char *deviceMAC, const char *childID, const char *alias);

    void init();
    void process_udp_incoming();
    void send_discover();

    void scan_and_refresh();

    uint64_t last_discover_at=0;

};

extern TPLinkScanner tp_link_scanner;

#endif //TPLINK_KASA_TPLINKSCANNER_H
