#ifndef TPLINK_KASA_TPLINKPLUG_H
#define TPLINK_KASA_TPLINKPLUG_H

#include <IPAddress.h>
#include "TPLinkDevice.h"


class TPLinkPlug : public TPLinkDevice {

public:
    TPLinkPlug(IPAddress ip, const char * deviceMAC, const char * deviceID, const char * childID, TPLinkConnector* tplink_conn);
    TPLinkPlug(IPAddress ip, const char * deviceMAC, const char * deviceID, const char * childID, const char * devAlias, TPLinkConnector* tplink_conn);

    bool last_read_on;

    bool set_on();
    bool set_off();

    void get_countdown();
    void set_countdown(uint8_t act, uint16_t secs);
    void clear_countdown();

    char child_id[3];

};


#endif //TPLINK_KASA_TPLINKPLUG_H
