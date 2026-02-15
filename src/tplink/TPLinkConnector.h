#ifndef TPLINK_KASA_TPLINKCONNECTOR_H
#define TPLINK_KASA_TPLINKCONNECTOR_H

#include <string>
#include <stdint.h>

#define TP_LINK_INITIALIZATION_VECTOR 171

class TPLinkConnector {

public:
    ~TPLinkConnector();

    void discover();

    void init_udp();

    void send_payload(uint32_t host_ip, const std::string payload, bool include_size);
    void broadcast_payload(std::string payload, bool include_size);
    std::string receive_udp();
    std::string receive_udp(uint32_t *udp_ip);

// private:
    static std::string encrypt(std::string unencr, bool include_size);
    static std::string decrypt(std::string encr);

private:
    int _sock_fd = -1;

};


#endif //TPLINK_KASA_TPLINKCONNECTOR_H
