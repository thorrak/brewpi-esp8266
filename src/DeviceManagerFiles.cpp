#include <ArduinoJson.h>
#include "DeviceManager.h"
#include "EepromStructs.h"
#include "ESPEepromAccess.h"
#include "JsonKeys.h"
#include "PiLink.h"

#ifdef HAS_BLUETOOTH
#include <NimBLEDevice.h>
#include "wireless/BTScanner.h"
#endif

#ifdef EXTERN_SENSOR_ACTUATOR_SUPPORT
#include "tplink/TPLinkScanner.h"
#endif


void DeviceConfig::toJson(JsonDocument &doc) {
    // Load the settings into the JSON Doc
    doc[DeviceDefinitionKeys::chamber] = chamber;
    doc[DeviceDefinitionKeys::beer] = beer;

    doc[DeviceDefinitionKeys::function] = deviceFunction;
    doc[DeviceDefinitionKeys::hardware] = deviceHardware;

    doc[DeviceDefinitionKeys::pin] = hw.pinNr;
    doc[DeviceDefinitionKeys::invert] = hw.invert;
    doc[DeviceDefinitionKeys::deactivated] = hw.deactivate;

    if(deviceHardware == DEVICE_HARDWARE_PIN) {
        switch(hw.pinNr) {
            case coolingPin:
                doc[DeviceDefinitionKeys::alias] = "Cool";
                break;
            case heatingPin:
                doc[DeviceDefinitionKeys::alias] = "Heat";
                break;
            case doorPin:
                doc[DeviceDefinitionKeys::alias] = "Door";
                break;
        }
    } else if(deviceHardware == DEVICE_HARDWARE_ONEWIRE_TEMP) {
        // copyArray doesn't work here since we don't want a JSON array, we want a string
        // If we ever want to switch to outputting an array, we can do so (but we need to ensure backwards
        // compatibility in brewpi-script)
        // copyArray(hw.address, doc[DeviceDefinitionKeys::address]);
        char buf[17];
        printBytes(hw.address, 8, buf);
        doc[DeviceDefinitionKeys::address] = buf;
    }
#ifdef HAS_BLUETOOTH
    else if(deviceHardware == DEVICE_HARDWARE_BLUETOOTH_INKBIRD || deviceHardware == DEVICE_HARDWARE_BLUETOOTH_TILT) {
        doc[DeviceDefinitionKeys::address] = hw.btAddress.toString();
        if(deviceHardware == DEVICE_HARDWARE_BLUETOOTH_TILT) {
            tilt *th = bt_scanner.get_tilt(hw.btAddress);
            if(th != nullptr) {
                // ArduinoJson attempts to deduplicate strings - we explicitly do not want that here
                // char color[20];
                // strcpy(color, th->get_color_string().c_str());
                doc[DeviceDefinitionKeys::alias] = String(th->get_color_string().c_str());
            }
        }
    }
#endif

#ifdef EXTERN_SENSOR_ACTUATOR_SUPPORT
    else if(deviceHardware == DEVICE_HARDWARE_TPLINK_SWITCH) {
        doc[DeviceDefinitionKeys::address] = hw.tplink_mac;
        doc[DeviceDefinitionKeys::child_id] = hw.tplink_child_id;
        TPLinkPlug *tp = tp_link_scanner.get_tplink_plug(hw.tplink_mac, hw.tplink_child_id);
        if(tp != nullptr)
            doc[DeviceDefinitionKeys::alias] = String(tp->device_alias);  // Prevent deduplication
    }
#endif

	if (deviceHardware==DEVICE_HARDWARE_ONEWIRE_TEMP || deviceHardware==DEVICE_HARDWARE_BLUETOOTH_INKBIRD || deviceHardware==DEVICE_HARDWARE_BLUETOOTH_TILT) {
		char buf[17];
		tempDiffToString(buf, temperature(hw.calibration<<(TEMP_FIXED_POINT_BITS - TEMP_CALIBRATION_OFFSET_PRECISION)), 3, 8);
    	doc[DeviceDefinitionKeys::calibrateadjust] = buf;
	}
}


void DeviceConfig::fromJson(JsonDocument json_doc) {

    // Load the settings from the JSON Doc
    if(json_doc[DeviceDefinitionKeys::chamber].is<uint8_t>()) chamber = json_doc[DeviceDefinitionKeys::chamber];
    if(json_doc[DeviceDefinitionKeys::beer].is<uint8_t>()) beer = json_doc[DeviceDefinitionKeys::beer];
    if(json_doc[DeviceDefinitionKeys::function].is<uint8_t>()) deviceFunction = (DeviceFunction) json_doc[DeviceDefinitionKeys::function];
    if(json_doc[DeviceDefinitionKeys::hardware].is<uint8_t>()) deviceHardware = (DeviceHardware) json_doc[DeviceDefinitionKeys::hardware];

    if(json_doc[DeviceDefinitionKeys::pin].is<uint8_t>()) hw.pinNr = json_doc[DeviceDefinitionKeys::pin];
    if(json_doc[DeviceDefinitionKeys::invert].is<bool>()) hw.invert = json_doc[DeviceDefinitionKeys::invert];
//    if(json_doc[DeviceDefinitionKeys::deactivated].is<bool>()) hw.deactivate = json_doc[DeviceDefinitionKeys::deactivated];
    hw.deactivate = false;


    if(deviceHardware == DEVICE_HARDWARE_ONEWIRE_TEMP && json_doc[DeviceDefinitionKeys::address].is<JsonArray>()) {
        // piLink.print("Loading using native address (JsonArray)");
        // piLink.printNewLine();
        copyArray(json_doc[DeviceDefinitionKeys::address], hw.address);
    } else if(deviceHardware == DEVICE_HARDWARE_ONEWIRE_TEMP && json_doc[DeviceDefinitionKeys::address].is<const char *>()) {
        // piLink.print("Loading using string address");
        // piLink.printNewLine();
        parseBytes(hw.address, json_doc[DeviceDefinitionKeys::address].as<const char *>(), 8);
        copyArray(json_doc[DeviceDefinitionKeys::address], hw.address);
#ifdef HAS_BLUETOOTH
    } else if(json_doc[DeviceDefinitionKeys::address].is<std::string>() && (deviceHardware == DEVICE_HARDWARE_BLUETOOTH_INKBIRD)) {
        // Inkbirds use address type 0 ("public") which (incorrectly!) indicates they bought a MAC block
        hw.btAddress = NimBLEAddress(json_doc[DeviceDefinitionKeys::address].as<std::string>(), 0);
    } else if(json_doc[DeviceDefinitionKeys::address].is<std::string>() && (deviceHardware == DEVICE_HARDWARE_BLUETOOTH_TILT)) {
        // Tilts use address type 1 ("random", which (correctly!) indicates they didn't buy a MAC block)
        hw.btAddress = NimBLEAddress(json_doc[DeviceDefinitionKeys::address].as<std::string>(), 1);
#endif
#ifdef EXTERN_SENSOR_ACTUATOR_SUPPORT
    } else if(json_doc[DeviceDefinitionKeys::address].is<const char *>() && json_doc[DeviceDefinitionKeys::child_id].is<const char *>() && (deviceHardware == DEVICE_HARDWARE_TPLINK_SWITCH)) {
		snprintf(hw.tplink_mac, 18, "%s", json_doc[DeviceDefinitionKeys::address].as<const char *>());
		snprintf(hw.tplink_child_id, 3, "%s", json_doc[DeviceDefinitionKeys::child_id].as<const char *>());
#endif
    }

	if (json_doc[DeviceDefinitionKeys::calibrateadjust].is<const char *>()) {
        hw.calibration = fixed4_4(stringToTempDiff(json_doc[DeviceDefinitionKeys::calibrateadjust].as<const char *>()) >> (TEMP_FIXED_POINT_BITS - TEMP_CALIBRATION_OFFSET_PRECISION));
	}

    // {
    //     String output;
    //     serializeJson(json_doc, output);
    //     piLink.print("Device fromJson: ");
    //     piLink.print(output);
    //     piLink.printNewLine();
    // }


}


void DeviceConfig::deviceFilename(char * fname, uint8_t devid) {
    sprintf(fname, "/dev%d", devid);
}


void DeviceConfig::storeToSpiffs(uint8_t devID) {
    JsonDocument doc;
    char fname[32];
    deviceFilename(fname, devID);

    toJson(doc);

    writeJsonToFile(fname, doc);  // Write the json to the file
}


void DeviceConfig::loadFromSpiffs(uint8_t devID) {
    char fname[32];
    deviceFilename(fname, devID);

    // We start by setting the defaults, as we use them as the alternative to loaded values if the keys (or file!) don't exist
    setDefaults();

    if(FILESYSTEM.exists(fname)) {
        JsonDocument json_doc = readJsonFromFile(fname);
        fromJson(json_doc);
    }
}

void DeviceConfig::setDefaults() {
    // Load the settings from the JSON Doc
    chamber = 0;
    beer = 0;

    deviceFunction = DEVICE_NONE;
    deviceHardware = DEVICE_HARDWARE_NONE;

    hw.pinNr = 0;
    hw.invert = false;
    hw.deactivate = false;

    hw.calibration = 0;
}
