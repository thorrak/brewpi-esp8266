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
}; // namespace DeviceDisplayKeys
