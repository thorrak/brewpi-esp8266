#include "TPLinkConnector.h"
#include "TPLinkPlug.h"

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include <WiFiUdp.h>



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


void TPLinkConnector::broadcast_payload(const std::string payload, bool include_size) {
    std::string encrypted_payload;

    encrypted_payload = encrypt(payload, include_size);

    //Send a packet to the targeted address consisting of the payload
    udp.beginPacket(UDP_BROADCAST_ADDR, UDP_TPLINK_PORT);
    udp.write((uint8_t*) encrypted_payload.c_str(), encrypted_payload.length());
    udp.endPacket();
}

void TPLinkConnector::send_payload(const IPAddress host, const std::string payload, bool include_size) {
    std::string encrypted_payload;

    encrypted_payload = encrypt(payload, include_size);

    //Send a packet to the broadcast address consisting of the payload
    udp.beginPacket(host, UDP_TPLINK_PORT);
    udp.write((uint8_t*) encrypted_payload.c_str(), encrypted_payload.length());
    udp.endPacket();
}



void TPLinkConnector::discover() {
    // Newer HS103s require the "short" discovery payload
    // const std::string discover_payload = "{\"system\": {\"get_sysinfo\": null}, \"emeter\": {\"get_realtime\": null}, \"smartlife.iot.dimmer\": {\"get_dimmer_parameters\": null}, \"smartlife.iot.common.emeter\": {\"get_realtime\": null}, \"smartlife.iot.smartbulb.lightingservice\": {\"get_light_state\": null}}";
    const std::string discover_payload = "{\"system\":{\"get_sysinfo\":{}}}";
    broadcast_payload(discover_payload, false);
    return;
}

void TPLinkConnector::init_udp() {
#ifdef ESP32
    udp.begin(WiFi.localIP(), UDP_TPLINK_PORT);
#elif defined(ESP8266)
    udp.begin(UDP_TPLINK_PORT);
#endif

}

std::string TPLinkConnector::receive_udp() {
    return receive_udp(nullptr);
}

std::string TPLinkConnector::receive_udp(IPAddress *udp_ip) {
    int packetSize;

    char incomingPacket[4096];  // buffer for incoming packets
    std::string packet_cppstr = "";  // std::string buffer for incoming packets

    packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(incomingPacket, 4096);
        if (len > 0) {
            incomingPacket[len] = 0;
        }
        packet_cppstr = incomingPacket;
        packet_cppstr = decrypt(packet_cppstr).c_str();
    }

    yield();
    if(udp_ip != nullptr)
        *udp_ip = udp.remoteIP();
    return packet_cppstr;
}
