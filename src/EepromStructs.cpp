
#include <FS.h>
#if defined(ESP32)
#include <SPIFFS.h>
#endif

#include <ArduinoJson.h>

#include "EepromStructs.h"
#include "TemperatureFormats.h"
#include "TempControl.h" // For definition of MODE_OFF



void JSONSaveable::writeJsonToFile(const char *filename, const ArduinoJson::JsonDocument& json_doc) {
    File file_out = SPIFFS.open(filename, "w");
    if (!file_out) {
        // If the above fails, we weren't able to open the file for writing
        return;
    }

    if (serializeJson(json_doc, file_out) == 0) {
        // Failed to write to the file
    }

    file_out.close();
}


ArduinoJson::DynamicJsonDocument JSONSaveable::readJsonFromFile(const char *filename) {
    DynamicJsonDocument json_doc(2048);

    File file_in = SPIFFS.open(filename, "r");
    if (!file_in) {
        // If the above fails, we weren't able to open the file for writing
        return json_doc;
    }

    deserializeJson(json_doc, file_in);

    file_in.close();
    return json_doc;
}



/**
 * \brief Constructor
 *
 * \see setDefaults
 */
ControlConstants::ControlConstants() {
    setDefaults();
}

/**
 * \brief Set reasonable default values for control constants
 */
void ControlConstants::setDefaults() {
    tempSettingMin = intToTemp(1);	// +1 deg Celsius
    tempSettingMax = intToTemp(30);	// +30 deg Celsius

    // control defines, also in fixed point format (7 int bits, 9 frac bits), so multiplied by 2^9=512
    Kp = intToTempDiff(5);	// +5
    Ki = intToTempDiff(1)/4; // +0.25
    Kd = intToTempDiff(-3)/2;	// -1.5
    iMaxError = intToTempDiff(5)/10;  // 0.5 deg

    // Stay Idle when fridge temperature is in this range
    idleRangeHigh = intToTempDiff(1);	// +1 deg Celsius
    idleRangeLow = intToTempDiff(-1);	// -1 deg Celsius

    // when peak falls between these limits, its good.
    heatingTargetUpper = intToTempDiff(3)/10;	// +0.3 deg Celsius
    heatingTargetLower = intToTempDiff(-2)/10;	// -0.2 deg Celsius
    coolingTargetUpper = intToTempDiff(2)/10;	// +0.2 deg Celsius
    coolingTargetLower = intToTempDiff(-3)/10;	// -0.3 deg Celsius

    // maximum history to take into account, in seconds
    maxHeatTimeForEstimate = 600;
    maxCoolTimeForEstimate = 1200,

    // Set filter coefficients. This is the b value. See FilterFixed.h for delay times.
    // The delay time is 3.33 * 2^b * number of cascades
    fridgeFastFilter = 1u;
    fridgeSlowFilter = 4u;
    fridgeSlopeFilter = 3u;
    beerFastFilter = 3u;
    beerSlowFilter = 4u;
    beerSlopeFilter = 4u;

    lightAsHeater = 0;
    rotaryHalfSteps = 0;
    pidMax = intToTempDiff(10);	// +/- 10 deg Celsius
    tempFormat = 'C';

}


/**
 * \brief Serialize control constants to JSON
 */
DynamicJsonDocument ControlConstants::toJson() {
    DynamicJsonDocument doc(1024);  // Should be a max of 642, per the ArduinoJson Size Assistant

    // Load the constants into the JSON Doc
    doc["tempSettingMin"] = tempToInt(tempSettingMin);
    doc["tempSettingMax"] = tempToInt(tempSettingMax);

    doc["Kp"] = tempDiffToInt(Kp);
    doc["Ki"] = tempDiffToInt(Ki);
    doc["Kd"] = tempDiffToInt(Kd);
    doc["iMaxError"] = tempDiffToInt(iMaxError);

    doc["idleRangeHigh"] = tempDiffToInt(idleRangeHigh);
    doc["idleRangeLow"] = tempDiffToInt(idleRangeLow);

    doc["heatingTargetUpper"] = tempDiffToInt(heatingTargetUpper);
    doc["heatingTargetLower"] = tempDiffToInt(heatingTargetLower);
    doc["coolingTargetUpper"] = tempDiffToInt(coolingTargetUpper);
    doc["coolingTargetLower"] = tempDiffToInt(coolingTargetLower);

    doc["maxHeatTimeForEstimate"] = maxHeatTimeForEstimate;
    doc["maxCoolTimeForEstimate"] = maxCoolTimeForEstimate;

    doc["fridgeFastFilter"] = fridgeFastFilter;
    doc["fridgeSlowFilter"] = fridgeSlowFilter;
    doc["fridgeSlopeFilter"] = fridgeSlopeFilter;
    doc["beerFastFilter"] = beerFastFilter;
    doc["beerSlowFilter"] = beerSlowFilter;
    doc["beerSlopeFilter"] = beerSlopeFilter;

    doc["lightAsHeater"] = lightAsHeater;
    doc["rotaryHalfSteps"] = rotaryHalfSteps;
    doc["pidMax"] = tempDiffToInt(pidMax);
    doc["tempFormat"] = tempFormat;
    doc["tempFormat"].is<char>();

    // Return the JSON document
    return doc;
}

void ControlConstants::storeToSpiffs() {
    DynamicJsonDocument doc(1024);  // Should be a max of 642, per the ArduinoJson Size Assistant

    doc = toJson();

    writeJsonToFile(ControlConstants::filename, doc);  // Write the json to the file
}

void ControlConstants::loadFromSpiffs() {
    // We start by setting the defaults, as we use them as the alternative to loaded values if the keys don't exist
    setDefaults();

    DynamicJsonDocument json_doc(2048);
    json_doc = readJsonFromFile(ControlConstants::filename);

    // Load the constants from the JSON Doc
    if(json_doc.containsKey("tempSettingMin"))
        tempSettingMin = intToTemp(json_doc["tempSettingMin"]);
    if(json_doc.containsKey("tempSettingMax"))
        tempSettingMax = intToTemp(json_doc["tempSettingMax"]);

    if(json_doc.containsKey("Kp"))
        Kp = intToTempDiff(json_doc["Kp"]);
    if(json_doc.containsKey("Ki"))
        Ki = intToTempDiff(json_doc["Ki"]);
    if(json_doc.containsKey("Kd"))
        Kd = intToTempDiff(json_doc["Kd"]);
    if(json_doc.containsKey("iMaxError"))
        iMaxError = intToTempDiff(json_doc["iMaxError"]);

    if(json_doc.containsKey("idleRangeHigh"))
        idleRangeHigh = intToTempDiff(json_doc["idleRangeHigh"]);
    if(json_doc.containsKey("idleRangeLow"))
        idleRangeLow = intToTempDiff(json_doc["idleRangeLow"]);

    if(json_doc.containsKey("heatingTargetUpper"))
        heatingTargetUpper = intToTempDiff(json_doc["heatingTargetUpper"]);
    if(json_doc.containsKey("heatingTargetLower"))
        heatingTargetLower = intToTempDiff(json_doc["heatingTargetLower"]);
    if(json_doc.containsKey("coolingTargetUpper"))
        coolingTargetUpper = intToTempDiff(json_doc["coolingTargetUpper"]);
    if(json_doc.containsKey("coolingTargetLower"))
        coolingTargetLower = intToTempDiff(json_doc["coolingTargetLower"]);

    maxHeatTimeForEstimate = json_doc["maxHeatTimeForEstimate"] | maxHeatTimeForEstimate;
    maxCoolTimeForEstimate = json_doc["maxCoolTimeForEstimate"] | maxCoolTimeForEstimate;

    fridgeFastFilter = json_doc["fridgeFastFilter"] | fridgeFastFilter;
    fridgeSlowFilter = json_doc["fridgeSlowFilter"] | fridgeSlowFilter;
    fridgeSlopeFilter = json_doc["fridgeSlopeFilter"] | fridgeSlopeFilter;
    beerFastFilter = json_doc["beerFastFilter"] | beerFastFilter;
    beerSlowFilter = json_doc["beerSlowFilter"] | beerSlowFilter;
    beerSlopeFilter = json_doc["beerSlopeFilter"] | beerSlopeFilter;

    lightAsHeater = json_doc["lightAsHeater"] | lightAsHeater;
    rotaryHalfSteps = json_doc["rotaryHalfSteps"] | rotaryHalfSteps;
    if(json_doc.containsKey("pidMax"))
        pidMax = intToTempDiff(json_doc["pidMax"]);
    tempFormat = json_doc["tempFormat"] | tempFormat;

}




/**
 * \brief Constructor
 * \see setDefaults
 */
ControlSettings::ControlSettings() {
    // When creating an instance of the class, set defaults. Loading is done explicitly.
    setDefaults();
}


/**
 * \brief Set reasonable defaults for the control settings
 */
void ControlSettings::setDefaults() {
    // TODO - Check if I need to do setMode here
    beerSetting = intToTemp(20);
    fridgeSetting = intToTemp(20);
    heatEstimator = intToTempDiff(2)/10; // 0.2
    coolEstimator=intToTempDiff(5);
    mode = MODE_OFF;  // We do NOT do call set_mode here - that is handled in TempControl::loadDefaultSettings()
}


DynamicJsonDocument ControlSettings::toJson() {
    DynamicJsonDocument doc(512);

    // Load the settings into the JSON Doc
    doc["beerSetting"] = tempToInt(beerSetting);
    doc["fridgeSetting"] = tempToInt(fridgeSetting);
    doc["heatEstimator"] = tempDiffToInt(heatEstimator);
    doc["coolEstimator"] = tempDiffToInt(coolEstimator);
    doc["mode"] = mode;
    doc["mode"].is<char>();

    // Return the JSON document
    return doc;
}


void ControlSettings::storeToSpiffs() {
    DynamicJsonDocument doc(512);

    doc = toJson();

    writeJsonToFile(ControlSettings::filename, doc);  // Write the json to the file
}


void ControlSettings::loadFromSpiffs() {
    // We start by setting the defaults, as we use them as the alternative to loaded values if the keys don't exist
    setDefaults();

    DynamicJsonDocument json_doc(1024);
    json_doc = readJsonFromFile(ControlSettings::filename);

    // Load the settings from the JSON Doc
    if(json_doc.containsKey("beerSetting"))
        beerSetting = intToTemp(json_doc["beerSetting"]);
    if(json_doc.containsKey("fridgeSetting"))
        fridgeSetting = intToTemp(json_doc["fridgeSetting"]);
    if(json_doc.containsKey("heatEstimator"))
        heatEstimator = intToTempDiff(json_doc["heatEstimator"]);
    if(json_doc.containsKey("coolEstimator"))
        coolEstimator = intToTempDiff(json_doc["coolEstimator"]);

    mode = json_doc["mode"] | mode;
}

