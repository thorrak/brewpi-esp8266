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
#include <type_traits>

#include "Brewpi.h"
#include "DeviceManager.h"
#include "DeviceNameManager.h"
#include "Logger.h"
#include "PiStream.h"
#include "TempControl.h"
#include "TemperatureFormats.h"
#include <stdarg.h>

class DeviceConfig;

/**
 * \brief Interface between brewpi controller and the outside world (Commonly a
 * RaspberryPi, hence the name).
 */
template <typename StreamType> class PiLink : public PiStream<StreamType> {
  static_assert(std::is_base_of<Stream, StreamType>::value, "StreamType must be a Stream");

public:
  PiLink(StreamType &stream) : PiStream<StreamType>(stream) {}

  /**
   * \brief Notify the Pi of the current state.
   *
   * Forcibly sends the current temperature information to cause data logging.
   * Optionally includes an annotation.
   *
   * \param beerAnnotation - Annotation for the beer
   * \param fridgeAnnotation - Annotation for the beer
   */
  void sendStateNotification(const char *beerAnnotation = nullptr, const char *fridgeAnnotation = nullptr) {
    printTemperatures(beerAnnotation, fridgeAnnotation);
  }

  /**
   * \brief Send a status message containing the current temperatures
   *
   * Can include optional annotations.  The monitoring script will process
   * and log this data.  While this is also the response to the request for
   * temperature ('t') command, it is used as a general purpose alert to the
   * controller script.  (This is why it is included in PiLink, and not
   * CommandProcessor, despite the latter seeming to be a more logical
   * location
   * \param beerAnnotation - Annotation for the beer
   * \param fridgeAnnotation - Annotation for the beer
   */
  void printTemperatures(const char *beerAnnotation, const char *fridgeAnnotation) {
    StaticJsonDocument<1024> doc;

    doc["BeerTemp"] = tempToDouble(tempControl.getBeerTemp(), Config::TempFormat::tempDecimals);
    doc["BeerSet"] = tempToDouble(tempControl.getBeerSetting(), Config::TempFormat::fixedPointDecimals);

    doc["BeerAnn"] = beerAnnotation;

    doc["FridgeTemp"] = tempToDouble(tempControl.getFridgeTemp(), Config::TempFormat::tempDecimals);
    doc["FridgeSet"] = tempToDouble(tempControl.getFridgeSetting(), Config::TempFormat::fixedPointDecimals);

    doc["FridgeAnn"] = fridgeAnnotation;

    if (tempControl.ambientSensor->isConnected()) {
      doc["RoomTemp"] = tempToDouble(tempControl.getRoomTemp(), Config::TempFormat::tempDecimals);
    } else {
      doc["RoomTemp"] = "";
    }

    doc["State"] = tempControl.getState();

#if BREWPI_SIMULATE
    doc["Time"] = ticks.millis() / 1000;
#endif

    this->sendJsonMessage('T', doc);
  }
};

#include "PiLinkCompatible.h"

// This will become a PiLink when everything has been migrated
extern CompatiblePiLink<std::conditional<Config::PiLink::useWifi, WiFiClient, HardwareSerial>::type> piLink;
