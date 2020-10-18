/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
 * Copyright 2013 Matthew McGowan.
 * Copyright 2020 Scott Peshak
 *
 * This file is part of BrewPi.
 *
 * BrewPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BrewPi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BrewPi.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SettingLoader.h"
#include "Display.h"
#include "PiLink.h"
#include "TempControl.h"

/**
 * \brief Process a single setting key/value pair
 *
 * \param kv - The parsed JsonPair of the setting
 */
void SettingLoader::processSettingKeypair(JsonPair kv) {
  // A good chunk of the conversions want a string representation of the value,
  // but the brewpi script presents the data as a number.  Prep a string
  // version in case we need it for this value.
  String str_value;
  if (kv.value().is<char *>())
    str_value = kv.value().as<char *>();
  else if (kv.value().is<float>()) {
    str_value = kv.value().as<float>();
  }

  if (kv.key() == "mode") {
    char mode = kv.value().as<char *>()[0];

    if (mode == Modes::fridgeConstant || mode == Modes::beerConstant || mode == Modes::beerProfile ||
        mode == Modes::off || mode == Modes::test) {
      tempControl.setMode(mode);
    } else {
      piLink.print_fmt("Invalid mode \"%c\" (0x%02X)", mode, mode);
      piLink.printNewLine();
    }
  }

  else if (kv.key() == "beerSet") {
    setBeerSetting(str_value.c_str());
  }

  else if (kv.key() == "fridgeSet") {
    setFridgeSetting(str_value.c_str());
  }

  else if (kv.key() == "heatEst") {
    tempControl.cs.heatEstimator = stringToFixedPoint(str_value.c_str());
  }

  else if (kv.key() == "coolEst") {
    tempControl.cs.coolEstimator = stringToFixedPoint(str_value.c_str());
  }

  else if (kv.key() == "tempFormat") {
    char format = kv.value().as<char *>()[0];

    if (format == 'C' || format == 'F') {
      tempControl.cc.tempFormat = format;
      // reprint stationary text to update to right degree unit
      display.printStationaryText();
    } else {
      piLink.print_P(PSTR("Invalid temp format \"%c\" (0x%02X)"), format, format);
      piLink.printNewLine();
    }
  }

  else if (kv.key() == "tempSetMin") {
    tempControl.cc.tempSettingMin = stringToTemp(str_value.c_str());
  }

  else if (kv.key() == "tempSetMax") {
    tempControl.cc.tempSettingMax = stringToTemp(str_value.c_str());
  }

  else if (kv.key() == "pidMax") {
    tempControl.cc.pidMax = stringToTempDiff(str_value.c_str());
  }

  else if (kv.key() == "Kp") {
    tempControl.cc.Kp = stringToFixedPoint(str_value.c_str());
  }

  else if (kv.key() == "Ki") {
    tempControl.cc.Ki = stringToFixedPoint(str_value.c_str());
  }

  else if (kv.key() == "Kd") {
    tempControl.cc.Kd = stringToFixedPoint(str_value.c_str());
  }

  else if (kv.key() == "iMaxErr") {
    tempControl.cc.iMaxError = stringToTempDiff(str_value.c_str());
  }

  else if (kv.key() == "idleRangeH") {
    tempControl.cc.idleRangeHigh = stringToTempDiff(str_value.c_str());
  }

  else if (kv.key() == "idleRangeL") {
    tempControl.cc.idleRangeLow = stringToTempDiff(str_value.c_str());
  }

  else if (kv.key() == "heatTargetH") {
    tempControl.cc.heatingTargetUpper = stringToTempDiff(str_value.c_str());
  }

  else if (kv.key() == "heatTargetL") {
    tempControl.cc.heatingTargetLower = stringToTempDiff(str_value.c_str());
  }

  else if (kv.key() == "coolTargetH") {
    tempControl.cc.coolingTargetUpper = stringToTempDiff(str_value.c_str());
  }

  else if (kv.key() == "coolTargetL") {
    tempControl.cc.coolingTargetLower = stringToTempDiff(str_value.c_str());
  }

  else if (kv.key() == "maxHeatTimeForEst") {
    tempControl.cc.maxHeatTimeForEstimate = kv.value().as<uint16_t>();
  }

  else if (kv.key() == "maxCoolTimeForEst") {
    tempControl.cc.maxCoolTimeForEstimate = kv.value().as<uint16_t>();
  }

  else if (kv.key() == "fridgeFastFilt") {
    tempControl.fridgeSensor->setFastFilterCoefficients(kv.value().as<uint8_t>());
  }

  else if (kv.key() == "fridgeSlowFilt") {
    tempControl.fridgeSensor->setSlowFilterCoefficients(kv.value().as<uint8_t>());
  }

  else if (kv.key() == "fridgeSlopeFilt") {
    tempControl.fridgeSensor->setSlopeFilterCoefficients(kv.value().as<uint8_t>());
  }

  else if (kv.key() == "beerFastFilt") {
    tempControl.beerSensor->setFastFilterCoefficients(kv.value().as<uint8_t>());
  }

  else if (kv.key() == "beerSlowFilt") {
    tempControl.beerSensor->setSlowFilterCoefficients(kv.value().as<uint8_t>());
  }

  else if (kv.key() == "beerSlopeFilt") {
    tempControl.beerSensor->setSlopeFilterCoefficients(kv.value().as<uint8_t>());
  }

  else if (kv.key() == "lah") {
    tempControl.cc.lightAsHeater = kv.value().as<bool>();
  }

  else if (kv.key() == "hs") {
    tempControl.cc.rotaryHalfSteps = kv.value().as<bool>();
  }
}

/**
 * Set the target beer temperature
 * @param val - New temp value
 */
void SettingLoader::setBeerSetting(const char *val) {
  String annotation = "Beer temp set to ";
  annotation += val;

  temperature newTemp = stringToTemp(val);

  if (tempControl.cs.mode == 'p') {
    // this excludes gradual updates under 0.2 degrees
    if (abs(newTemp - tempControl.cs.beerSetting) > 100) {
      annotation += " by temperature profile";
    }
  } else {
    annotation += " in web interface";
  }

  if (annotation.length() > 0)
    piLink.sendStateNotification(annotation.c_str());

  tempControl.setBeerTemp(newTemp);
}

/**
 * Set the target fridge temperature
 * @param val - New temp value
 */
void SettingLoader::setFridgeSetting(const char *val) {
  temperature newTemp = stringToTemp(val);
  if (tempControl.cs.mode == 'f') {
    String annotation = "Fridge temp set to ";
    annotation += val;
    annotation += " in web interface";

    piLink.sendStateNotification(nullptr, annotation.c_str());
  }

  tempControl.setFridgeTemp(newTemp);
}
