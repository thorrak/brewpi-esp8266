#include "TPLinkConnector.h"
#include "TPLinkPlug.h"

#include "ESP_BP_WiFi.h"

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <errno.h>

#include <string>
#include <stdexcept>
#include <vector>
using namespace std;

#define UDP_BROADCAST_ADDR  "255.255.255.255"
#define UDP_TPLINK_PORT     9999



std::string string_to_hex(const std::string& input)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input)
    {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}


int hex_value(char hex_digit)
{
    switch (hex_digit) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return hex_digit - '0';

        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            return hex_digit - 'A' + 10;

        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            return hex_digit - 'a' + 10;
    }
    return 0;
}

std::string hex_to_string(const std::string& input)
{
    const auto len = input.length();
    std::string output;

    if (len & 1) {
        return output;  // TODO - Do something better here
    }

    output.reserve(len / 2);
    for (auto it = input.begin(); it != input.end(); )
    {
        int hi = hex_value(*it++);
        int lo = hex_value(*it++);
        output.push_back(hi << 4 | lo);
    }
    return output;
}




std::string intToBytestring(int paramInt)
{
    vector<unsigned char> arrayOfByte(4);
    for (int i = 0; i < 4; i++)
        arrayOfByte[3 - i] = (paramInt >> (i * 8));

    // recast to string & return
    std::string s(arrayOfByte.begin(), arrayOfByte.end());

    return s;
}

std::string TPLinkConnector::encrypt(std::string unencr, bool include_size) {
    unsigned char key = TP_LINK_INITIALIZATION_VECTOR;
    char encr_byte;
    std::string encr_string;

    // Recast a uint16_t containing the length of the unencrypted payload string to a byte string and load this into the
    // encr_string which we will ultimately transmit
    if(include_size)
        encr_string += intToBytestring((uint16_t) unencr.length());

    for (char const &c: unencr) {
        encr_byte = key ^ c;
        key = encr_byte;
        encr_string += encr_byte;
    }

    return encr_string;
}

std::string TPLinkConnector::decrypt(std::string encr) {
    unsigned char key = TP_LINK_INITIALIZATION_VECTOR;
    unsigned char decr_byte;
    std::string decr_string;

    for (char const &c: encr) {
        decr_byte = key ^ c;
        key = c;
        decr_string += decr_byte;
    }

    return decr_string;
}


TPLinkConnector::~TPLinkConnector() {
    if (_sock_fd >= 0) {
        lwip_close(_sock_fd);
        _sock_fd = -1;
    }
}


void TPLinkConnector::broadcast_payload(const std::string payload, bool include_size) {
    if (_sock_fd < 0) return;

    std::string encrypted_payload = encrypt(payload, include_size);

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(UDP_TPLINK_PORT);
    dest.sin_addr.s_addr = INADDR_BROADCAST;

    lwip_sendto(_sock_fd, encrypted_payload.c_str(), encrypted_payload.length(),
                0, (struct sockaddr *)&dest, sizeof(dest));
}

void TPLinkConnector::send_payload(uint32_t host_ip, const std::string payload, bool include_size) {
    if (_sock_fd < 0) return;

    std::string encrypted_payload = encrypt(payload, include_size);

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(UDP_TPLINK_PORT);
    dest.sin_addr.s_addr = host_ip;  // already network order

    lwip_sendto(_sock_fd, encrypted_payload.c_str(), encrypted_payload.length(),
                0, (struct sockaddr *)&dest, sizeof(dest));
}



void TPLinkConnector::discover() {
    // Newer HS103s require the "short" discovery payload
    const std::string discover_payload = "{\"system\":{\"get_sysinfo\":{}}}";
    broadcast_payload(discover_payload, false);
    return;
}

void TPLinkConnector::init_udp() {
    if (_sock_fd >= 0) {
        lwip_close(_sock_fd);
    }

    _sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_sock_fd < 0) return;

    // Enable broadcast
    int broadcast_en = 1;
    lwip_setsockopt(_sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_en, sizeof(broadcast_en));

    // Set non-blocking so receive_udp doesn't block
    int flags = lwip_fcntl(_sock_fd, F_GETFL, 0);
    lwip_fcntl(_sock_fd, F_SETFL, flags | O_NONBLOCK);

    // Bind to the TP-Link port on the local IP
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(UDP_TPLINK_PORT);
    local.sin_addr.s_addr = bp_wifi_get_ip_addr();

    lwip_bind(_sock_fd, (struct sockaddr *)&local, sizeof(local));
}

std::string TPLinkConnector::receive_udp() {
    return receive_udp(nullptr);
}

std::string TPLinkConnector::receive_udp(uint32_t *udp_ip) {
    if (_sock_fd < 0) return "";

    char incomingPacket[4096];
    struct sockaddr_in src;
    socklen_t src_len = sizeof(src);

    int len = lwip_recvfrom(_sock_fd, incomingPacket, sizeof(incomingPacket) - 1,
                            0, (struct sockaddr *)&src, &src_len);

    if (len <= 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        return "";
    }

    incomingPacket[len] = 0;
    std::string packet_cppstr(incomingPacket, len);
    packet_cppstr = decrypt(packet_cppstr);

    vTaskDelay(pdMS_TO_TICKS(1));

    if (udp_ip != nullptr)
        *udp_ip = src.sin_addr.s_addr;  // network-order uint32_t

    return packet_cppstr;
}
