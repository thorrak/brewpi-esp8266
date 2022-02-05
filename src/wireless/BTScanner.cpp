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

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

////////////////////////////
// BLE Scanner Callbacks/Code
////////////////////////////


void load_inkbird_from_advert(NimBLEAdvertisedDevice* advertisedDevice);

/** Handles callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        // Inkbird IBS-TH2
        if(advertisedDevice->getName().rfind("sps",0) == 0 && advertisedDevice->getManufacturerData().length() == 9) {
            // Log.verbose(F("Advertised Device: %s \r\n"), advertisedDevice->toString().c_str());
            load_inkbird_from_advert(advertisedDevice);
            return;
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

////////////////////////////
// btScanner Implementation
////////////////////////////

btScanner::btScanner()
{
    shouldRun = false;
    m_last_inkbird_purge_at = 0;
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
            // Log.verbose(F("Found matching Kegtron: %s \r\n"), kt.id);
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

#endif