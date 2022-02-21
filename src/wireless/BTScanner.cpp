#ifdef HAS_BLUETOOTH
#include <Arduino.h>
#include <bitset> // for std::bitset
#include <list>

#include "Ticker.h"
#include <ArduinoJson.h>
#include <NimBLEBeacon.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>

#include "BTScanner.h"
// #include "serialhandler.h"


// Create the scanner
btScanner bt_scanner;
std::list<inkbird> lInkbirds;
std::list<tilt> lTilts;

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

////////////////////////////
// BLE Scanner Callbacks/Code
////////////////////////////


void load_inkbird_from_advert(NimBLEAdvertisedDevice* advertisedDevice);
void load_tilt_from_advert(NimBLEAdvertisedDevice* advertisedDevice);

/** Handles callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        // Inkbird IBS-TH2
        if(advertisedDevice->getName().rfind("sps",0) == 0 && advertisedDevice->getManufacturerData().length() == 9) {
            // Log.verbose(F("Advertised Device: %s \r\n"), advertisedDevice->toString().c_str());
            load_inkbird_from_advert(advertisedDevice);
            return;
        } else if (advertisedDevice->getManufacturerData().length() >= 24) {  // Tilt
            if (advertisedDevice->getManufacturerData()[0] == 0x4c && advertisedDevice->getManufacturerData()[1] == 0x00 &&
                advertisedDevice->getManufacturerData()[2] == 0x02 && advertisedDevice->getManufacturerData()[3] == 0x15)
            {
                load_tilt_from_advert(advertisedDevice);
                return;
            }
        }
    };
};


void load_inkbird_from_advert(NimBLEAdvertisedDevice* advertisedDevice)
{
    // The advertisement string is the "manufacturer data" part of the following:
    // example: f208361300f28b6408
    // format:  tttthhhh??????bb??
    // tttt = 100* temp in C (needs endian change)
    // hhhh = 100 * humidity in C (needs endian change)
    // bb = battery in %

    // Decode temp/humidity/battery
    int16_t temp = (advertisedDevice->getManufacturerData()[1]<<8) + advertisedDevice->getManufacturerData()[0];
    uint16_t hum = (advertisedDevice->getManufacturerData()[3]<<8) + advertisedDevice->getManufacturerData()[2];
    uint8_t bat = advertisedDevice->getManufacturerData()[7];

    // Log.verbose(F("Detected Inkbird: Temp: %d, Hum: %d, Bat: %d\r\n"), temp, hum, bat);

    // Locate & update the inkbird object in the list
    inkbird *ib = bt_scanner.get_or_create_inkbird(advertisedDevice->getAddress());
    ib->update(temp, hum, bat, advertisedDevice->getRSSI());
    return;
}

void load_tilt_from_advert(NimBLEAdvertisedDevice* advertisedDevice)
{
    // The advertisement string is the "manufacturer data" part of the following:
    //Advertised Device: Name: Tilt, Address: 88:c2:55:ac:26:81, manufacturer data: 4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //????????iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiittttggggXR
    //**********----------**********----------**********
    TiltColor m_color;
    char hex_code[3] = {'\0'};
    char m_color_arr[33] = {'\0'};
    char temp_arr[5] = {'\0'};
    char grav_arr[5] = {'\0'};
    char tx_pwr_arr[3] = {'\0'};

    for (int i = 4; i < advertisedDevice->getManufacturerData().length(); i++)
    {
        sprintf(hex_code, "%.2x", advertisedDevice->getManufacturerData()[i]);

        if ((i > 3) && (i < 20))  // Indices 4 - 19 each generate two characters of the color array
            strncat(m_color_arr, hex_code, 2);
        else if (i == 20 || i == 21)  // Indices 20-21 each generate two characters of the temperature array
            strncat(temp_arr, hex_code, 2);
        else if (i == 22 || i == 23)  // Indices 22-23 each generate two characters of the sp_gravity array
            strncat(grav_arr, hex_code, 2);
        else if (i == 24)  // Index 24 contains the tx_pwr (which is used by recent tilts to indicate battery age)
            strncat(tx_pwr_arr, hex_code, 2);
    }

    m_color = tilt::uuid_to_color_no(m_color_arr);

    if(m_color==TiltColor::TILT_COLOR_NONE)  // No color was matched from the advert string
        return;

    uint16_t temp = std::strtoul(temp_arr, nullptr, 16);
    uint16_t gravity = std::strtoul(grav_arr, nullptr, 16);
    uint8_t tx_pwr = std::strtoul(tx_pwr_arr, nullptr, 16);

    // Log.verbose(F("Detected Tilt: Temp: %d, Hum: %d, Bat: %d\r\n"), temp, hum, bat);


    // Locate & update the tilt object in the list
    tilt *th = bt_scanner.get_or_create_tilt(advertisedDevice->getAddress());
    th->update(m_color, temp, gravity, tx_pwr, advertisedDevice->getRSSI());
}


////////////////////////////
// btScanner Implementation
////////////////////////////

btScanner::btScanner()
{
    shouldRun = false;
    m_last_inkbird_purge_at = 0;
    m_last_tilt_purge_at = 0;
}

void btScanner::init()
{

    /** Initialize NimBLE, no device name spcified as we are not advertising */
    NimBLEDevice::init("");

    shouldRun = true;
    /** Optional: set the transmit power, default is 3db */
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db  */
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();  // Create/get the scan
    // NOTE - The below probably creates a memory leak with deinit (but deinit is never called in our code).
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());  // Initialize the callbacks
    pBLEScan->setMaxResults(0);
    pBLEScan->setActiveScan(true); // Required for some devices - active scan actively queries devices for more info following detection.
    pBLEScan->setInterval(97); // Select prime numbers to reduce risk of frequency beat pattern with ibeacon advertisement interval
    pBLEScan->setWindow(37);   // Set to less or equal setInterval value. Leave reasonable gap to allow WiFi some time.
}


bool btScanner::scan()
{
    if (!shouldRun)
        return false;
    if (NimBLEDevice::getScan()->isScanning())
        return false;
    delay(500);
    if (NimBLEDevice::getScan()->start(BLE_SCAN_TIME, nullptr, false))
        return true; //Scan successfully started.
    return false;  //Scan failed to start
}

void btScanner::stop_scan()
{
    shouldRun = false;
    NimBLEDevice::getScan()->stop();
    NimBLEDevice::getScan()->clearResults();
}


inkbird* btScanner::get_inkbird(const NimBLEAddress devAddress)
{
    for(inkbird & ib : lInkbirds) {
        if(ib.deviceAddress == devAddress) {
            // Access the object through the iterator
            return &ib;
        }
    }

    return nullptr;
}

inkbird* btScanner::get_or_create_inkbird(const NimBLEAddress devAddress)
{
    inkbird *found_ib = get_inkbird(devAddress);

    if(found_ib)
        return found_ib;

    // No matching device was found
    inkbird newInkbird(devAddress);
    lInkbirds.push_front(newInkbird);

    return get_inkbird(devAddress);  // We specifically want to access the object as referenced in the list
}


tilt* btScanner::get_tilt(const NimBLEAddress devAddress)
{
    for(tilt & th : lTilts) {
        if(th.deviceAddress == devAddress) {
            // Access the object through the iterator
            return &th;
        }
    }

    return nullptr;
}

tilt* btScanner::get_or_create_tilt(const NimBLEAddress devAddress)
{
    tilt *found_th = get_tilt(devAddress);

    if(found_th)
        return found_th;

    // No matching device was found
    tilt newTilt(devAddress);
    lTilts.push_front(newTilt);

    return get_tilt(devAddress);  // We specifically want to access the object as referenced in the list
}

// void btScanner::purge_stale_inkbirds()
// {
//     // Check if we've passed the threshhold to purge devices
//     if(millis() < (1000 * BT_SCANNER_INKBIRD_PURGE_TIME + m_last_inkbird_purge_at))
//         return;

//     // We've passed the threshhold. Loop through devices and purge as appropriate
//     for(inkbird & ib : lInkbirds) {
//         if(millis() > (ib.m_lastUpdate + (BT_SCANNER_INKBIRD_PURGE_TIME * 1000) ))
//         {
//             lInkbirds.remove(ib);
//         }
//     }
//     m_last_inkbird_purge_at = millis();
// }

// void btScanner::purge_stale_tilts()
// {
//     // Check if we've passed the threshhold to purge devices
//     if(millis() < (1000 * BT_SCANNER_TILT_PURGE_TIME + m_last_tilt_purge_at))
//         return;

//     // We've passed the threshhold. Loop through devices and purge as appropriate
//     for(tilt & th : lTilts) {
//         if(millis() > (ib.m_lastUpdate + (BT_SCANNER_TILT_PURGE_TIME * 1000) ))
//         {
//             lTilts.remove(ib);
//         }
//     }
//     m_last_tilts_purge_at = millis();
// }


#endif