#ifndef TPLINK_KASA_TPLINKDEVICE_H
#define TPLINK_KASA_TPLINKDEVICE_H

#include <string>
#include "TPLinkConnector.h"
#include <IPAddress.h>

enum TPLinkDeviceType {
	TPLINK_KASA_SMARTPLUGSWITCH = 0 // e.g. HS103, KP400
};


class TPLinkDevice {
public:
    IPAddress ip_addr;
    TPLinkDeviceType type;

    char device_mac[18];
    char device_alias[32];

protected:
    TPLinkConnector* tp_link_connector;
    void send_payload(const std::string payload, bool include_size);
};


#endif //TPLINK_KASA_TPLINKDEVICE_H
