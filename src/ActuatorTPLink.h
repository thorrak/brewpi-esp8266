#ifndef BREWPI_ESP_ACTUATORTPLINK_H
#define BREWPI_ESP_ACTUATORTPLINK_H

#include "Brewpi.h"
#include "Actuator.h"
// #include "tplink/TPLinkScanner.h"
#include "PiLink.h"

/**
 * An actuator that operates by communicating with a TPLink Kasa Smart Switch device.
 *
 */
class TPLinkActuator : public Actuator
{
public:	

    TPLinkActuator(const char * deviceMAC, const char * childID);

    void setActive(bool active);

    bool isActive();
    

            
private:
    char device_mac[18];
    char child_id[3];
};

#endif