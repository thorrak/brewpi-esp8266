/*
 * Copyright 2020 Scott Peshak
 * Copyright 2013 BrewPi/Elco Jacobs.
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

#include "Brewpi.h"
#include <ArduinoJson.h>

/**
 * Tuple of device name and ID
 */
struct DeviceName
{
  String name;
  String device;

  DeviceName(String device, String name): name(name), device(device){}
};



/**
 * Class to manage human readable names for devices
 */
class DeviceNameManager
{
  public:
    static void setDeviceName(const char* device, const char* name);
    static String getDeviceName(const char* device);
    static void deleteDeviceName(const char* device);

    static void enumerateDeviceNames(JsonDocument& doc);


  private:
    static void deviceNameFilename(char* filename, const char* device);
    static DeviceName filenameToDeviceName(String filename);

    static const char filenamePrefix[];
    static constexpr int prefixLength();
};

