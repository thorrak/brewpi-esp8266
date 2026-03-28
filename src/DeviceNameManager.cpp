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


#include "DeviceNameManager.h"
#include "ESPEepromAccess.h"  // Includes filesystem headers/definition
#include <string>
#include <dirent.h>

/**
 * \brief Set a human readable name for a device.
 * \param device - The identifier for the device, most commonly the OneWire device address (in hex)
 * \param name - The name to set
 */
void DeviceNameManager::setDeviceName(const char* device, const char* name)
{
  char filename[32];
  DeviceNameManager::deviceNameFilename(filename, device);

  FILE* f = fs_open(filename, "w");

  if (f) {
    fputs(name, f);
    fclose(f);
  }
}


/**
 * \brief Get the human readable name for a device.
 *
 * If no name has been registered, the device ID will be returned.
 * \param device - The identifier for the device, most commonly the OneWire device address (in hex)
 * \return The registered device name, or if none is set, the provided device ID
 */
std::string DeviceNameManager::getDeviceName(const char* device) {
  char filename[32];
  DeviceNameManager::deviceNameFilename(filename, device);

  if (fs_exists(filename)) {
    FILE* f = fs_open(filename, "r");
    if (f) {
      char buf[64];
      size_t len = fread(buf, 1, sizeof(buf) - 1, f);
      buf[len] = '\0';
      fclose(f);
      return std::string(buf);
    }
  }

  return std::string(device);
}


/**
 * \brief Get the Filename that contains the device human name metadata for a given device.
 * \param device - The identifier for the device, most commonly the OneWire device address (in hex)
 */
inline void DeviceNameManager::deviceNameFilename(char* filename, const char* device) {
  strcpy(filename, DeviceNameManager::filenamePrefix);
  strncat(filename, device, 32 - DeviceNameManager::prefixLength());
}


/**
 * \brief Prefix used when building device name filenames
 *
 * This helps disambiguate the name files from other data, and makes it easier
 * to produce a list of named probes.
 */
const char DeviceNameManager::filenamePrefix[] = "/dn/";

/**
 * \brief Calculate length of DeviceNameManager::filenamePrefix
 *
 * This is done as a constexpr so it can be calculated at compile time
 */
constexpr int DeviceNameManager::prefixLength() {
  return strlen(DeviceNameManager::filenamePrefix);
}


/**
 * \brief Delete a configured device name
 * \param device - The identifier for the device, most commonly the OneWire device address (in hex)
 */
void DeviceNameManager::deleteDeviceName(const char* device) {
  char filename[32];
  DeviceNameManager::deviceNameFilename(filename, device);

  fs_remove(filename);
}


/**
 * \brief Get list of configured device names
 */
void DeviceNameManager::enumerateDeviceNames(JsonDocument& doc) {
  char fullpath[64];
  snprintf(fullpath, sizeof(fullpath), "%s%s", FS_PREFIX, filenamePrefix);

  DIR* dir = opendir(fullpath);
  if (!dir) return;

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr) {
    // Build the path as filenamePrefix + entry name, matching what filenameToDeviceName expects
    char entryPath[288];
    snprintf(entryPath, sizeof(entryPath), "%s%s", filenamePrefix, entry->d_name);
    DeviceName dn = filenameToDeviceName(entryPath);
    doc[dn.device] = dn.name;
  }
  closedir(dir);
}


/**
 * \brief Given a filename, get a DeviceName
 *
 * \param filename
 */
DeviceName DeviceNameManager::filenameToDeviceName(const char* filename) {
  // strip the prefix off
  const char* stripped = filename + prefixLength();

  std::string name = getDeviceName(stripped);
  return DeviceName(stripped, name.c_str());
}
