/*
 * Copyright 2013 Matthew McGowan
 * Copyright 2013 BrewPi/Elco Jacobs.
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

#pragma once

/**
 * \brief Strings used for JSON keys
 * \see DeviceDefinition
 */
namespace DeviceDefinitionKeys {
constexpr auto index = "i";
constexpr auto chamber = "c";
constexpr auto beer = "b";
constexpr auto function = "f";
constexpr auto hardware = "h";
constexpr auto pin = "p";
constexpr auto invert = "x";
constexpr auto deactivated = "d";
constexpr auto address = "a";
constexpr auto child_id = "n";
constexpr auto alias = "r";  // EXTERN_SENSOR_ACTUATOR or Bluetooth
#if BREWPI_DS2413
constexpr auto pio = "n";
#endif
constexpr auto calibrateadjust = "j";

constexpr auto value = "v";
constexpr auto write = "w";

constexpr auto type = "t";
}; // namespace DeviceDefinitionKeys


/**
 * \brief Strings used for JSON keys
 * \see DeviceDisplay
 */
namespace DeviceDisplayKeys {
constexpr auto index = "i";
constexpr auto value = "v";
constexpr auto write = "w";
constexpr auto empty = "e";
}; // namespace DeviceDisplayKeys



/**
 * \brief Strings used for JSON keys
 * \see EnumerateHardware
 */
namespace EnumerateHardwareKeys {
constexpr auto hardware = "h";
constexpr auto pin = "p";
constexpr auto values = "v";
constexpr auto unused = "u";
constexpr auto function = "f";
}; // namespace EnumerateHardwareKeys


/**
 * \brief Strings used for JSON keys
 * \see ControlSettings
 */
namespace ControlSettingsKeys {
constexpr auto beer = "beerSetting";
constexpr auto fridge = "fridgeSetting";
constexpr auto heatEst = "heatEstimator";
constexpr auto coolEst = "coolEstimator";
constexpr auto mode = "mode";
};


/**
 * \brief Strings used for JSON keys
 * \see ControlConstants
 */
namespace ControlConstantsKeys {
constexpr auto tempMin = "tempSettingMin";
constexpr auto tempMax = "tempSettingMax";
constexpr auto kp = "Kp";
constexpr auto ki = "Ki";
constexpr auto kd = "Kd";
constexpr auto maxError = "iMaxError";
constexpr auto idleHigh = "idleRangeHigh";
constexpr auto idleLow = "idleRangeLow";
constexpr auto heatingUpper = "heatingTargetUpper";
constexpr auto heatingLower = "heatingTargetLower";
constexpr auto coolingUpper = "coolingTargetUpper";
constexpr auto coolingLower = "coolingTargetLower";
constexpr auto maxHeatEst = "maxHeatTimeForEstimate";
constexpr auto maxCoolEst = "maxCoolTimeForEstimate";
constexpr auto fridgeFilterFast = "fridgeFastFilter";
constexpr auto fridgeFilterSlow = "fridgeSlowFilter";
constexpr auto fridgeFilterSlope = "fridgeSlopeFilter";
constexpr auto beerFilterFast = "beerFastFilter";
constexpr auto beerFilterSlow = "beerSlowFilter";
constexpr auto beerFilterSlope = "beerSlopeFilter";
constexpr auto lightHeater = "lightAsHeater";
constexpr auto rotaryHalfSteps = "rotaryHalfSteps";
constexpr auto pidMax = "pidMax";
constexpr auto tempFormat = "tempFormat";
};


/**
 * \brief Strings used for JSON keys
 * \see ExtendedSettings
 */
namespace ExtendedSettingsKeys {
constexpr auto eepromReset = "confirmReset";
constexpr auto invertTFT = "invertTFT";
constexpr auto glycol = "glycol";
constexpr auto largeTFT = "largeTFT";
}; // namespace ExtendedSettingsKeys


/**
 * \brief Strings used for JSON keys
 * \see UpstreamSettings
 */
namespace UpstreamSettingsKeys {
constexpr auto upstreamHost = "upstreamHost";
constexpr auto upstreamPort = "upstreamPort";
constexpr auto deviceID = "deviceID";
}; // namespace UpstreamSettingsKeys



/**
 * \brief Strings used for JSON keys
 * \see processUpdateModeJson
 */
namespace ModeUpdateKeys {
constexpr auto mode = "mode";
constexpr auto setpoint = "setPoint";
}; // namespace ModeUpdateKeys


