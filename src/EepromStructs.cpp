#ifdef ESP8266
#include <LittleFS.h>
#elif defined(ESP32)
#include <FS.h>
#include <SPIFFS.h>
#endif

#include <ArduinoJson.h>

#include "EepromStructs.h"
#include "TemperatureFormats.h"
#include "TempControl.h" // For Modes definition
#include "JsonKeys.h"
#include "PiLink.h"



void JSONSaveable::writeJsonToFile(const char *filename, const ArduinoJson::JsonDocument& json_doc) {
    File file_out = FILESYSTEM.open(filename, "w");

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

    File file_in = FILESYSTEM.open(filename, "r");
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
    doc[ControlConstantsKeys::tempMin] = tempSettingMin;
    doc[ControlConstantsKeys::tempMax] = tempSettingMax;

    doc[ControlConstantsKeys::kp] = Kp;
    doc[ControlConstantsKeys::ki] = Ki;
    doc[ControlConstantsKeys::kd] = Kd;
    doc[ControlConstantsKeys::maxError] = iMaxError;

    doc[ControlConstantsKeys::idleHigh] = idleRangeHigh;
    doc[ControlConstantsKeys::idleLow] = idleRangeLow;

    doc[ControlConstantsKeys::heatingUpper] = heatingTargetUpper;
    doc[ControlConstantsKeys::heatingLower] = heatingTargetLower;
    doc[ControlConstantsKeys::coolingUpper] = coolingTargetUpper;
    doc[ControlConstantsKeys::coolingLower] = coolingTargetLower;

    doc[ControlConstantsKeys::maxHeatEst] = maxHeatTimeForEstimate;
    doc[ControlConstantsKeys::maxCoolEst] = maxCoolTimeForEstimate;

    doc[ControlConstantsKeys::fridgeFilterFast] = fridgeFastFilter;
    doc[ControlConstantsKeys::fridgeFilterSlow] = fridgeSlowFilter;
    doc[ControlConstantsKeys::fridgeFilterSlope] = fridgeSlopeFilter;
    doc[ControlConstantsKeys::beerFilterFast] = beerFastFilter;
    doc[ControlConstantsKeys::beerFilterSlow] = beerSlowFilter;
    doc[ControlConstantsKeys::beerFilterSlope] = beerSlopeFilter;

    doc[ControlConstantsKeys::lightHeater] = lightAsHeater;
    doc[ControlConstantsKeys::rotaryHalfSteps] = rotaryHalfSteps;
    doc[ControlConstantsKeys::pidMax] = pidMax;
    doc[ControlConstantsKeys::tempFormat] = tempFormat;
    doc[ControlConstantsKeys::tempFormat].is<char>();

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
    if(json_doc.containsKey(ControlConstantsKeys::tempMin)) tempSettingMin = json_doc[ControlConstantsKeys::tempMin];
    if(json_doc.containsKey(ControlConstantsKeys::tempMax)) tempSettingMax = json_doc[ControlConstantsKeys::tempMax];

    if(json_doc.containsKey(ControlConstantsKeys::kp)) Kp = json_doc[ControlConstantsKeys::kp];
    if(json_doc.containsKey(ControlConstantsKeys::ki)) Ki = json_doc[ControlConstantsKeys::ki];
    if(json_doc.containsKey(ControlConstantsKeys::kd)) Kd = json_doc[ControlConstantsKeys::kd];
    if(json_doc.containsKey(ControlConstantsKeys::maxError)) iMaxError = json_doc[ControlConstantsKeys::maxError];

    if(json_doc.containsKey(ControlConstantsKeys::idleHigh)) idleRangeHigh = json_doc[ControlConstantsKeys::idleHigh];
    if(json_doc.containsKey(ControlConstantsKeys::idleLow)) idleRangeLow = json_doc[ControlConstantsKeys::idleLow];

    if(json_doc.containsKey(ControlConstantsKeys::heatingUpper))
        heatingTargetUpper = json_doc[ControlConstantsKeys::heatingUpper];
    if(json_doc.containsKey(ControlConstantsKeys::heatingLower))
        heatingTargetLower = json_doc[ControlConstantsKeys::heatingLower];
    if(json_doc.containsKey(ControlConstantsKeys::coolingUpper))
        coolingTargetUpper = json_doc[ControlConstantsKeys::coolingUpper];
    if(json_doc.containsKey(ControlConstantsKeys::coolingLower))
        coolingTargetLower = json_doc[ControlConstantsKeys::coolingLower];

    maxHeatTimeForEstimate = json_doc[ControlConstantsKeys::maxHeatEst] | maxHeatTimeForEstimate;
    maxCoolTimeForEstimate = json_doc[ControlConstantsKeys::maxCoolEst] | maxCoolTimeForEstimate;

    fridgeFastFilter = json_doc[ControlConstantsKeys::fridgeFilterFast] | fridgeFastFilter;
    fridgeSlowFilter = json_doc[ControlConstantsKeys::fridgeFilterSlow] | fridgeSlowFilter;
    fridgeSlopeFilter = json_doc[ControlConstantsKeys::fridgeFilterSlope] | fridgeSlopeFilter;
    beerFastFilter = json_doc[ControlConstantsKeys::beerFilterFast] | beerFastFilter;
    beerSlowFilter = json_doc[ControlConstantsKeys::beerFilterSlow] | beerSlowFilter;
    beerSlopeFilter = json_doc[ControlConstantsKeys::beerFilterSlope] | beerSlopeFilter;

    lightAsHeater = json_doc[ControlConstantsKeys::lightHeater] | lightAsHeater;
    rotaryHalfSteps = json_doc[ControlConstantsKeys::rotaryHalfSteps] | rotaryHalfSteps;
    if(json_doc.containsKey(ControlConstantsKeys::pidMax)) pidMax = json_doc[ControlConstantsKeys::pidMax];
    tempFormat = json_doc[ControlConstantsKeys::tempFormat] | tempFormat;
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
    mode = Modes::off;  // We do NOT do call set_mode here - that is handled in TempControl::loadDefaultSettings()
}


DynamicJsonDocument ControlSettings::toJson() {
    DynamicJsonDocument doc(512);

    // Load the settings into the JSON Doc
    doc[ControlSettingsKeys::beer] = beerSetting;
    doc[ControlSettingsKeys::fridge] = fridgeSetting;
    doc[ControlSettingsKeys::heatEst] = heatEstimator;
    doc[ControlSettingsKeys::coolEst] = coolEstimator;
    doc[ControlSettingsKeys::mode] = mode;
    doc[ControlSettingsKeys::mode].is<char>();

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
    if(json_doc.containsKey(ControlSettingsKeys::beer)) beerSetting = json_doc[ControlSettingsKeys::beer];
    if(json_doc.containsKey(ControlSettingsKeys::fridge)) fridgeSetting = json_doc[ControlSettingsKeys::fridge];
    if(json_doc.containsKey(ControlSettingsKeys::heatEst)) heatEstimator = json_doc[ControlSettingsKeys::heatEst];
    if(json_doc.containsKey(ControlSettingsKeys::coolEst)) coolEstimator = json_doc[ControlSettingsKeys::coolEst];

    mode = json_doc[ControlSettingsKeys::mode] | mode;
}

