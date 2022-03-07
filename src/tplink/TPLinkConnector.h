#ifndef TPLINK_KASA_TPLINKCONNECTOR_H
#define TPLINK_KASA_TPLINKCONNECTOR_H

#include <string>
#include <WiFiUdp.h>

#define TP_LINK_INITIALIZATION_VECTOR 171

class TPLinkConnector {

public:
    // TPLinkConnector();
    // ~TPLinkConnector();

    void discover();

    void init_udp();

    void send_payload(const IPAddress host, const std::string payload, bool include_size);
    void broadcast_payload(std::string payload, bool include_size);
    std::string receive_udp();
    std::string receive_udp(IPAddress *udp_ip);

// private:
    static std::string encrypt(std::string unencr, bool include_size);
    static std::string decrypt(std::string encr);

private:
    //The udp library class
    WiFiUDP udp;

};


#endif //TPLINK_KASA_TPLINKCONNECTOR_H
