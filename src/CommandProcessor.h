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
#pragma once

#include <ArduinoJson.h>

/**
 * \brief Functions that implement protocol commands
 */
struct CommandProcessor {
public:
  static void receiveCommand();

private:
  static void invalidCommand(const char inByte);
  static void commandNotImplemented(const char command, const String message);
  static void versionInfo();

  static void sendControlSettings();
  static void sendControlConstants();
  static void sendControlVariables();

  static void setDefaultConstants();
  static void setDefaultSettings();

  static void processSettingsJson();

  static void printTemperatures();
  static void printRawTemperatures();

  // Hardware state
  static void setAlarmState(bool enabled);
  static void listDevices();
  static void listHardware();
  static void resetWiFi();
  static void wifiInfo();

  // LCD
  static void toggleBacklight();
  static void getLcdContent();

  // EEPROM
  static void initEeprom();

  // Device name
  static void printDeviceNames();
  static void setDeviceNames();

  static void processSettingKeypair(JsonPair);
};
