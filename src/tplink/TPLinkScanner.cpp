#include <ArduinoJson.h>
#include "TPLinkScanner.h"
#include "TPLinkPlug.h"

#include "Config.h"
#include "EepromStructs.h"
#include "EepromManager.h"

#include <esp_wifi.h>
#include "ESP_BP_WiFi.h"
#include <esp_timer.h>


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

TPLinkPlug* TPLinkScanner::get_or_create_tplink_plug(uint32_t ip_addr, const char *deviceMAC, const char *deviceID, const char *childID, const char *alias)
{
    TPLinkPlug *found_tp = get_tplink_plug(deviceMAC, childID);

    if(found_tp) {
        found_tp->ip_addr = ip_addr;
        if(strcmp(alias, found_tp->device_alias) != 0)  // Update the alias if it changed
            snprintf(found_tp->device_alias, 32, "%s", alias);
        return found_tp;
    }

    // No matching device was found
    TPLinkPlug newTPLinkPlug(ip_addr, deviceMAC, deviceID, childID, &tplink_connector);
    lTPLinkPlugs.push_front(newTPLinkPlug);

    return get_tplink_plug(deviceMAC, childID);  // We specifically want to access the object as referenced in the list
}


void TPLinkScanner::process_udp_incoming() {
    uint32_t udp_ip = 0;
    std::string incoming_packet;

    uint64_t scan_until = (unsigned long)(esp_timer_get_time() / 1000ULL) + (5 * 1000); // Read replies for 5 seconds at most
    while((unsigned long)(esp_timer_get_time() / 1000ULL) < scan_until) {
        incoming_packet = tplink_connector.receive_udp(&udp_ip);

        if(incoming_packet.length() <= 0)
            return; // No data returned

        // Serial.printf("process_udp_incoming: Received %s from IP address %s\n", incoming_packet.c_str(), udp_ip.toString().c_str());


        JsonDocument json_doc;
        deserializeJson(json_doc, incoming_packet);


        if(json_doc["system"].is<JsonObject>() && json_doc["system"]["get_sysinfo"].is<JsonObject>()) {
            // This is a response to a scan
                        
            // Recent TP Link devices have a type rather than a mic_type
            if(!(json_doc["system"]["get_sysinfo"]["mic_type"].is<const char *>() || json_doc["system"]["get_sysinfo"]["type"].is<const char *>())) 
                continue;  // Invalid, unable to process
                        
            if(!json_doc["system"]["get_sysinfo"]["mac"].is<const char *>() || !json_doc["system"]["get_sysinfo"]["deviceId"].is<const char *>()) {
                // Serial.println("Invalid packet - unable to process");
                // Serial.printf("process_udp_incoming: Received %s from IP address %s\n", incoming_packet.c_str(), udp_ip.toString().c_str());
                continue;  // Invalid, unable to process
            }

            if((json_doc["system"]["get_sysinfo"]["mic_type"].is<const char *>() && json_doc["system"]["get_sysinfo"]["mic_type"] == "IOT.SMARTPLUGSWITCH") ||
                (json_doc["system"]["get_sysinfo"]["type"].is<const char *>() && json_doc["system"]["get_sysinfo"]["type"] == "IOT.SMARTPLUGSWITCH")) 
            {
                TPLinkPlug *this_plug;
                // This is a smart switch - process against the switch list
                if(!json_doc["system"]["get_sysinfo"]["child_num"].is<int>()) {
                    // If we're missing the child_num key, this is a single-plug switch
                    this_plug = get_or_create_tplink_plug(udp_ip, json_doc["system"]["get_sysinfo"]["mac"].as<const char *>(), json_doc["system"]["get_sysinfo"]["deviceId"].as<const char *>(), "", json_doc["system"]["get_sysinfo"]["alias"].as<const char *>());

                    if(json_doc["system"]["get_sysinfo"]["relay_state"].as<int>() == 0)
                        this_plug->last_read_on = false;
                    else
                        this_plug->last_read_on = true;

                } else {
                    // child_num is present and interpretable
                    if (!json_doc["system"]["get_sysinfo"]["children"].is<JsonArray>())
                        continue; // Children has to be populated to loop through them

                    // Need to loop through the children
                    for(JsonObject plug_doc : json_doc["system"]["get_sysinfo"]["children"].as<JsonArray>()) {
                        if(!plug_doc["id"].is<const char *>() || !plug_doc["state"].is<int>()) {
                            continue;  // Invalid, unable to process
                        }
                        this_plug = get_or_create_tplink_plug(udp_ip, json_doc["system"]["get_sysinfo"]["mac"].as<const char *>(), json_doc["system"]["get_sysinfo"]["deviceId"].as<const char *>(), plug_doc["id"].as<const char *>(), plug_doc["alias"].as<const char *>());

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

void TPLinkScanner::send_refresh() {
#ifdef EXTERN_SENSOR_ACTUATOR_SUPPORT
    DeviceConfig dc;

    // We need to check which devices are assigned, and ensure that they are TPLink Switches
    for (int8_t idx=0; idx<Config::EepromFormat::MAX_DEVICES; idx++) {
        dc = eepromManager.fetchDevice(idx);
        if(dc.deviceHardware == DEVICE_HARDWARE_TPLINK_SWITCH) {
            // Run through plugs that are on, clear the timer, and refresh the timer
            TPLinkPlug *tp = get_tplink_plug(dc.hw.tplink_mac, dc.hw.tplink_child_id);
            if(tp && tp->last_read_on) {
                // Countdown is cleared automatically by set_countdown
                // tp.clear_countdown();
                tp->set_countdown_to_off(TPLINK_SAFETY_TIMEOUT);
            }
        }
    }
#endif
}

void TPLinkScanner::scan_and_refresh() {

    if(!bp_wifi_is_connected())
        return;

    if((unsigned long)(esp_timer_get_time() / 1000ULL) > (last_discover_at + (TPLINK_DISCOVER_EVERY * 1000))) {
        tp_link_scanner.send_discover();
        last_discover_at = (unsigned long)(esp_timer_get_time() / 1000ULL);
    }

    if((unsigned long)(esp_timer_get_time() / 1000ULL) > (last_refresh_at + (TPLINK_SAFETY_TIMEOUT_REFRESH * 1000))) {
        tp_link_scanner.send_refresh();
        last_refresh_at = (unsigned long)(esp_timer_get_time() / 1000ULL);
    }

    tp_link_scanner.process_udp_incoming();
}
