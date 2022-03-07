
#include <Arduino.h>
#include <ArduinoJson.h>
#include "TPLinkScanner.h"
#include "TPLinkPlug.h"


TPLinkScanner tp_link_scanner;

void TPLinkScanner::init() {
    tplink_connector.init_udp();
}


TPLinkPlug* TPLinkScanner::get_tplink_plug(const char *deviceMAC, const char *childID)
{
    for(TPLinkPlug & tp : lTPLinkPlugs) {
        if(strcmp(tp.child_id, childID) == 0 && strcmp(tp.device_mac, deviceMAC) == 0) {
            // Access the object through the iterator
            return &tp;
        }
    }
    return nullptr;
}

TPLinkPlug* TPLinkScanner::get_or_create_tplink_plug(IPAddress ip_addr, const char *deviceMAC, const char *childID, const char *alias)
{
    TPLinkPlug *found_tp = get_tplink_plug(deviceMAC, childID);

    if(found_tp) {
        found_tp->ip_addr = ip_addr;
        if(strcmp(alias, found_tp->device_alias) != 0)  // Update the alias if it changed
            snprintf(found_tp->device_alias, 32, "%s", alias);
        return found_tp;
    }

    // No matching device was found
    TPLinkPlug newTPLinkPlug(ip_addr, deviceMAC, childID, &tplink_connector);
    lTPLinkPlugs.push_front(newTPLinkPlug);

    return get_tplink_plug(deviceMAC, childID);  // We specifically want to access the object as referenced in the list
}


void TPLinkScanner::process_udp_incoming() {
    IPAddress udp_ip;
    std::string incoming_packet;

    uint64_t scan_until = xTaskGetTickCount() + (5 * 1000); // Read replies for 5 seconds at most 
    while(xTaskGetTickCount() < scan_until) {
        incoming_packet = tplink_connector.receive_udp(&udp_ip);

        if(incoming_packet.length() <= 0)
            return; // No data returned

        Serial.printf("process_udp_incoming: Received %s from IP address %s\n", incoming_packet.c_str(), udp_ip.toString().c_str());


        DynamicJsonDocument json_doc(4096);
        deserializeJson(json_doc, incoming_packet);


        if(json_doc.containsKey("system") && json_doc["system"].is<JsonObject>() && json_doc["system"].containsKey("get_sysinfo") && json_doc["system"]["get_sysinfo"].is<JsonObject>()) {
            // This is a response to a scan
            if(!json_doc["system"]["get_sysinfo"].containsKey("mic_type") || !json_doc["system"]["get_sysinfo"]["mic_type"].is<std::string>() ||
            !json_doc["system"]["get_sysinfo"].containsKey("mac") || !json_doc["system"]["get_sysinfo"]["mac"].is<std::string>()) {
                // Serial.println("Invalid packet - unable to process");
                // Serial.printf("process_udp_incoming: Received %s from IP address %s\n", incoming_packet.c_str(), udp_ip.toString().c_str());
                continue;  // Invalid, unable to process
            }

            if(json_doc["system"]["get_sysinfo"]["mic_type"] == "IOT.SMARTPLUGSWITCH") {
                TPLinkPlug *this_plug;
                // This is a smart switch - process against the switch list
                if(!json_doc["system"]["get_sysinfo"].containsKey("child_num")) {
                    // If we're missing the child_num key, this is a single-plug switch
                    this_plug = get_or_create_tplink_plug(udp_ip, json_doc["system"]["get_sysinfo"]["mac"].as<std::string>().c_str(), "", json_doc["system"]["get_sysinfo"]["alias"].as<std::string>().c_str());

                    if(json_doc["system"]["get_sysinfo"]["relay_state"].as<int>() == 0)
                        this_plug->last_read_on = false;
                    else
                        this_plug->last_read_on = true;

                } else if(json_doc["system"]["get_sysinfo"]["child_num"].is<int>()) {
                    if (!json_doc["system"]["get_sysinfo"].containsKey("children") || !json_doc["system"]["get_sysinfo"]["children"].is<JsonArray>())
                        continue; // Children has to be populated to loop through them

                    // Need to loop through the children
                    for(JsonObject plug_doc : json_doc["system"]["get_sysinfo"]["children"].as<JsonArray>()) {
                        if(!plug_doc.containsKey("id") || !plug_doc["id"].is<std::string>() ||
                        !plug_doc.containsKey("state") || !plug_doc["state"].is<int>()) {
                            continue;  // Invalid, unable to process
                        }
                        this_plug = get_or_create_tplink_plug(udp_ip, json_doc["system"]["get_sysinfo"]["mac"].as<std::string>().c_str(), plug_doc["id"].as<std::string>().c_str(), plug_doc["alias"].as<std::string>().c_str());

                        // Update the cached state
                        if(plug_doc["state"].as<int>() == 0)
                            this_plug->last_read_on = false;
                        else
                            this_plug->last_read_on = true;
                    }
                }
            } else {
                continue;  // For now, we can't handle any Kasa acceessories other than SMARTPLUGSWITCH
            }
        } else {
            // This is not a scan response 
            // Serial.println("Missing keys - unable to process.");
            continue;
        }
    }

    return;
}


void TPLinkScanner::send_discover() {
    tplink_connector.discover();
}


void TPLinkScanner::scan_and_refresh() {
    if(millis() > (last_discover_at + (TPLINK_DISCOVER_EVERY * 1000))) {
        tp_link_scanner.send_discover();
        last_discover_at = millis();
    }

    // TODO - Loop through and process safety timeouts here

    tp_link_scanner.process_udp_incoming();
}
