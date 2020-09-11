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


#include <FS.h>
#if defined(ESP32)
#include <SPIFFS.h>
#endif

#include "DeviceNameManager.h"

/**
 * Set a human readable name for a device.
 * @param device - The identifier for the device, most commonly the OneWire device address (in hex)
 * @param name - The name to set
 */
void DeviceNameManager::setDeviceName(const char* device, const char* name)
{
  char filename[32];
  DeviceNameManager::deviceNameFilename(filename, device);

  File f = SPIFFS.open(filename, "w");
  if (f) {
    f.print(name);
    f.close();
  }
}


/**
 * Get the human readable name for a device.
 * @param device - The identifier for the device, most commonly the OneWire device address (in hex)
 */
String DeviceNameManager::getDeviceName(const char* device) {
  char filename[32];
  DeviceNameManager::deviceNameFilename(filename, device);

  if (SPIFFS.exists(filename)) {
    File f = SPIFFS.open(filename, "r");
    if (f) {
      String res = f.readString();
      f.close();
      return res;
    }
  }

  return "";
}


/**
 * Get the Filename that contains the device human name metadata for a given device.
 * @param device - The identifier for the device, most commonly the OneWire device address (in hex)
 */
inline void DeviceNameManager::deviceNameFilename(char* filename, const char* device) {
  strcpy(filename, DeviceNameManager::filenamePrefix);
  strncat(filename, device, 32 - DeviceNameManager::prefixLength());
}


/**
 * Prefix used when building device name filenames
 *
 * This helps disambiguate the name files from other data, and makes it easier
 * to produce a list of named probes.
 */
const char DeviceNameManager::filenamePrefix[] = "/dn/";

/**
 * Calculate length of filename prefix.
 * This is done as a constexpr so it can be calculated at compile time
 */
constexpr int DeviceNameManager::prefixLength() {
  return strlen(DeviceNameManager::filenamePrefix);
}


/**
 * Delete a configured device name
 * @param device - The identifier for the device, most commonly the OneWire device address (in hex)
 */
void DeviceNameManager::deleteDeviceName(const char* device) {
  char filename[32];
  DeviceNameManager::deviceNameFilename(filename, device);

  SPIFFS.remove(filename);
}


#if defined(ESP32)
/**
 * Get list of configured device names
 */
void DeviceNameManager::enumerateDeviceNames(deviceNameHandler callback) {
  File root = SPIFFS.open(filenamePrefix);

  File file = root.openNextFile();

  while(file){
    callback(filenameToDeviceName(file.name()));

    // Move to the next file
    file = root.openNextFile();
  }
}

#else

/**
 * Get list of configured device names
 */
void DeviceNameManager::enumerateDeviceNames(deviceNameHandler callback) {
  Dir dir = SPIFFS.openDir(filenamePrefix);

  while (dir.next()) {
    callback(filenameToDeviceName(dir.fileName()));
  }
}
#endif


/**
 * Given a filename, get a DeviceName
 *
 * @param filename
 */
DeviceName DeviceNameManager::filenameToDeviceName(String filename) {
  // strip the prefix off
  filename = filename.substring(prefixLength());

  return DeviceName(filename, getDeviceName(filename.c_str()));
}
