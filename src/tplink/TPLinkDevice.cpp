#include "TPLinkDevice.h"

void TPLinkDevice::send_payload(const std::string payload, bool include_size) {
    // This is a thin wrapper around tp_link_connector's send_payload that just adds the IP address to the send request
    tp_link_connector->send_payload(ip_addr, payload, include_size);
}

TPLinkDevice::TPLinkDevice() {
    device_mac[0] = '\0';
    device_alias[0] = '\0';
    device_id[0] = '\0';
}