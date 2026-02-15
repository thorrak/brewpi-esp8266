/*
* Copyright 2016 John Beeler
*
* This is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this file.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <esp_littlefs.h>

// Mount point prefix for all filesystem paths
#define FS_PREFIX "/littlefs"

// Initialize the LittleFS filesystem via ESP-IDF VFS
// Call once during setup(), before any file operations.
bool filesystem_init(bool format_if_failed = true);

// Check if a file exists
static inline bool fs_exists(const char* path) {
    char fullpath[64];
    snprintf(fullpath, sizeof(fullpath), "%s%s", FS_PREFIX, path);
    struct stat st;
    return (stat(fullpath, &st) == 0);
}

// Remove a file
static inline bool fs_remove(const char* path) {
    char fullpath[64];
    snprintf(fullpath, sizeof(fullpath), "%s%s", FS_PREFIX, path);
    return (::remove(fullpath) == 0);
}

// Open a file (returns FILE*). Caller must fclose() the result.
static inline FILE* fs_open(const char* path, const char* mode) {
    char fullpath[64];
    snprintf(fullpath, sizeof(fullpath), "%s%s", FS_PREFIX, path);
    return fopen(fullpath, mode);
}


#include "EepromStructs.h"
#include "Brewpi.h"  // Only needed for Config:: below


class ESPEepromAccess
{
private:
    static bool doesFileExist(const char* target_name) {
        return fs_exists(target_name);
    }

public:

    static void zapData() {
        // This gets a bit tricky -- we can't just do FS.format because that would wipe out the mDNS name
        int i;
        if(doesFileExist(ControlConstants::filename)) fs_remove(ControlConstants::filename);
        if(doesFileExist(ControlSettings::filename)) fs_remove(ControlSettings::filename);

        char buf[20];
        for(i=0;i<Config::EepromFormat::MAX_DEVICES;i++) {
			DeviceConfig::deviceFilename(buf, i);  // Get the filename from the function in the class
            if(doesFileExist(buf)) fs_remove(buf);
        }
    }
};
