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

#define FILESYSTEM LittleFS
#include <LittleFS.h>

#include "EepromStructs.h"
#include "Brewpi.h"  // Only needed for Config:: below


class ESPEepromAccess
{
private:
    static bool doesFileExist(String target_name) {
        return FILESYSTEM.exists(target_name);
    }

public:

    static void zapData() {
        // This gets a bit tricky -- we can't just do FS.format because that would wipe out the mDNS name
        int i;
        if(doesFileExist(ControlConstants::filename)) FILESYSTEM.remove(ControlConstants::filename);
        if(doesFileExist(ControlSettings::filename)) FILESYSTEM.remove(ControlSettings::filename);

        char buf[20];
        for(i=0;i<Config::EepromFormat::MAX_DEVICES;i++) {
			DeviceConfig::deviceFilename(buf, i);  // Get the filename from the function in the class
            if(doesFileExist(buf)) FILESYSTEM.remove(buf);
        }
    }
};
