
#include "Brewpi.h"
#include "Pins.h"
#include "ActuatorTPLink.h"
#include "tplink/TPLinkScanner.h"



TPLinkActuator::TPLinkActuator(const char * deviceMAC, const char * childID) {
    strcpy(device_mac, deviceMAC);
    strcpy(child_id, childID);
}

void TPLinkActuator::setActive(bool active) {
    TPLinkPlug *tp;

    tp = tp_link_scanner.get_tplink_plug(device_mac, child_id);

    if(!tp) // Unable to find the actuator in the list, we can't set active.
        return;

    if(active && !isActive())
        tp->set_on();
    else if(!active && isActive())
        tp->set_off();
}

bool TPLinkActuator::isActive() {
    TPLinkPlug *tp;

    tp = tp_link_scanner.get_tplink_plug(device_mac, child_id);

    if(!tp) // Unable to find the actuator in the list, lets assume it's turned off
        return false;

    return tp->last_read_on;
}
