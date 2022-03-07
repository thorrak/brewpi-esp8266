#include <Arduino.h>
#include <WiFiUdp.h>
#include "TPLinkPlug.h"
#include <string>


TPLinkPlug::TPLinkPlug(IPAddress ip, const char * deviceMAC, const char * childID, TPLinkConnector* tplink_conn) {
    if(strlen(deviceMAC) <= 17)
        strcpy(device_mac, deviceMAC);

    if(strlen(childID) <= 3)
        strcpy(child_id, childID);

    ip_addr = ip;
    tp_link_connector = tplink_conn;
    last_read_on = false;
}


TPLinkPlug::TPLinkPlug(IPAddress ip, const char * deviceMAC, const char * childID, const char * devAlias, TPLinkConnector* tplink_conn) {
    if(strlen(deviceMAC) <= 17)
        strcpy(device_mac, deviceMAC);

    snprintf(device_alias, 32, "%s", devAlias);

    if(strlen(childID) <= 3)
        strcpy(child_id, childID);

    ip_addr = ip;
    tp_link_connector = tplink_conn;
    last_read_on = false;
}


bool TPLinkPlug::set_on() {
    const std::string command = "{\"system\": {\"set_relay_state\": {\"state\": 1}}}";
    send_payload(command, false);
    last_read_on = true;  // Update the cache to assume that we processed successfully. Alternatively, we can test for the response message. 
    return true;
}


bool TPLinkPlug::set_off() {
    const std::string command = "{\"system\": {\"set_relay_state\": {\"state\": 0}}}";
    send_payload(command, false);
    last_read_on = false;  // Update the cache to assume that we processed successfully. Alternatively, we can test for the response message. 
    return true;
}


void TPLinkPlug::get_countdown() {
    const std::string command = "{\"count_down\":{\"get_rules\":null}}";
    send_payload(command, false);
    return;


    // uint64_t scan_until;

    // int packetSize;
    // int devices_found=0;

    // char incomingPacket[2048];  // buffer for incoming packets
    // std::string packet_cppstr;  // std::string buffer for incoming packets

    // const std::string off_cmd = "{\"count_down\":{\"get_rules\":null}}";

    // // for (int i = 0; i < 3; i++) {
    // send_payload(off_cmd, false);
    // // }

    // delay(100);  // Wait 100ms for devices to respond


    // // Then start polling for discovered devices
    // scan_until = xTaskGetTickCount() + (5 * 1000); // Read replies for 5 seconds
    // while(xTaskGetTickCount() < scan_until) {
    //     packetSize = udp.parsePacket();
    //     if (packetSize) {
    //         // receive incoming UDP packets
    //         Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(),
    //                       udp.remotePort());
    //         int len = udp.read(incomingPacket, 2048);
    //         if (len > 0) {
    //             incomingPacket[len] = 0;
    //         }
    //         packet_cppstr = incomingPacket;
    //         packet_cppstr = TPLinkConnector::decrypt(packet_cppstr).c_str();
    //         Serial.printf("UDP packet contents: %s\n", packet_cppstr.c_str());
    //         devices_found++;

    //         // send back a reply, to the IP address and port we got the packet from
    // //        udp.beginPacket(udp.remoteIP(), udp.remotePort());
    // //        udp.write(replyPacket);
    // //        udp.endPacket();
    //     }

    //     yield();
    // }

    return;
}

void TPLinkPlug::set_countdown(uint8_t act, uint16_t secs) {
    char command[128];
    snprintf(command, 128, "{\"count_down\":{\"add_rule\":{\"enable\":1,\"delay\":%d,\"act\":%d,\"name\":\"take action\"}}}", secs, act);

    // Before we send the command to set a new countdown, clear any countdown that may already exist
    clear_countdown();
    delay(50);

    send_payload(command, false);
    return;
}

void TPLinkPlug::clear_countdown() {
    const std::string command = "{\"count_down\":{\"delete_all_rules\":null}}";
    send_payload(command, false);
    return;
}


