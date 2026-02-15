#ifndef TPLINK_KASA_TPLINKDEVICE_H
#define TPLINK_KASA_TPLINKDEVICE_H

#include <string>
#include <stdint.h>
#include "TPLinkConnector.h"

enum TPLinkDeviceType {
	TPLINK_KASA_SMARTPLUGSWITCH = 0 // e.g. HS103, KP400
};


class TPLinkDevice {
public:
    uint32_t ip_addr;  // network-order IPv4 address
    TPLinkDeviceType type;

    char device_mac[18];
    char device_alias[32];
    char device_id[41];

    TPLinkDevice();

protected:
    TPLinkConnector* tp_link_connector;
    void send_payload(const std::string payload, bool include_size);
};


#endif //TPLINK_KASA_TPLINKDEVICE_H
