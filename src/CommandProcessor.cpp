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

#include "CommandProcessor.h"
#include "Brewpi.h"
#include "Display.h"
#include "PiLink.h"
#include "PromServer.h"
#include "SettingLoader.h"
#include "SettingsManager.h"
#include "TempControl.h"
#include "Version.h"
#include <ArduinoJson.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#if BREWPI_SIMULATE == 1
#include "Simulator.h"
#endif

extern void handleReset();

/**
 * \brief Receive incoming commands
 *
 * Continuously reads data from PiStream and processes the command strings.
 */
void CommandProcessor::receiveCommand() {
  while (piLink.available() > 0) {
    char inByte = piLink.read();

    // Clamp the command (roughly) to the printable ASCII range
    // This cuts down the number of cases needed in the switch and silence
    // noise caused by telnet control characters.  Command values within the
    // range will cause a message to be returned (see the default case of the
    // switch)
    if (inByte < 65 || inByte > 122) {
      continue;
    }

    // The command cases are sorted alphabetically
    switch (inByte) {
    case 'A': // alarm on
      setAlarmState(true);
      break;

    case 'a': // alarm off
      setAlarmState(false);
      break;

    case 'b': // Toggle Backlight (since we don't have a rotary encoder)
      toggleBacklight();
      break;

    case 'C': // Set default constants
      setDefaultConstants();
      break;

    case 'c': // Control constants requested
      sendControlConstants();
      break;

    case 'd': // list devices in eeprom order
      listDevices();
      break;

    case 'E': // initialize eeprom
      initEeprom();
      break;

    case 'h': // hardware query
      listHardware();
      break;

    case 'j': // Receive settings as json
      processSettingsJson();
      break;

    case 'l': // Display content requested
      getLcdContent();
      break;

    case 'n': // Hardware & firmware version information
      versionInfo();
      break;

    case 'o': // Configure probe names
      setDeviceNames();
      break;

    case 'p': // Get configured probe names
      printDeviceNames();
      break;

    case 'R': // reset
      handleReset();
      break;

    case 'S': // Set default settings
      setDefaultSettings();
      break;

    case 's': // Control settings requested
      sendControlSettings();
      break;

    case 'T': // All probe temps
      printRawTemperatures();
      break;

    case 't': // temperatures requested
      printTemperatures();
      break;

    case 'U': // update device
      deviceManager.parseDeviceDefinition();
      break;

    case 'v': // Control variables requested
      sendControlVariables();
      break;

    case 'w': // Reset WiFi settings
      resetWiFi();
      break;

    case 'W': // WiFi connection info
      wifiInfo();
      break;

#if BREWPI_SIMULATE == 1
    case 'Y':
      simulator.printSettings();
      break;

    case 'y':
      simulator.parseSettings();
      break;
#endif

    // Alert the user that the command issued wasn't valid
    default:
      invalidCommand(inByte);
    }
  }
}

/**
 * \brief Display a warning about unknown commands
 * \param inByte - The received command
 */
void CommandProcessor::invalidCommand(const char inByte) {
  piLink.print_P(PSTR("Invalid command received \"%c\" (0x%02X)"), inByte, inByte);
  piLink.printNewLine();
}

/**
 * \brief Inform user that the requested command isn't implemented.
 *
 * This differs from an invalid command because an invalid command is not ever
 * valid in the command set.  Not-implemented commands are valid, it's just
 * that the code doesn't currently support the command.  The lack of support
 * could be due to configuration or just a yet to be finished feature.
 *
 * \param command - Command requested.
 * \param message - An message explaining why the command isn't implemented.
 */
void CommandProcessor::commandNotImplemented(const char command, const String message) {
  piLink.print_P(PSTR("Command \"%c\" not implemented. %s"), command, message.c_str());
  piLink.printNewLine();
}

/**
 * \brief Version information
 *
 * Sends a string with software & hardware version information.
 * \ingroup commands
 */
void CommandProcessor::versionInfo() {
  DynamicJsonDocument doc(256);
  // v version
  // s shield type
  // y: simulator
  // b: board

  doc["v"] = Config::Version::release;
  doc["n"] = Config::Version::git_rev;
  doc["c"] = Config::Version::git_tag;
  doc["s"] = BREWPI_STATIC_CONFIG;
  doc["y"] = BREWPI_SIMULATE;
  doc["b"] = String(BREWPI_BOARD);
  doc["l"] = BREWPI_LOG_MESSAGES_VERSION;

  piLink.sendJsonMessage('N', doc);
}

/**
 * \brief Reset control constants to their default values.
 *
 * \ingroup commands
 */
void CommandProcessor::setDefaultConstants() {
  tempControl.loadDefaultConstants();
  display.printStationaryText(); // reprint stationary text to update to right
                                 // degree unit
  sendControlConstants();        // update script with new settings
  logInfo(INFO_DEFAULT_CONSTANTS_LOADED);
}

/**
 * \brief Reset settings to their default values.
 *
 * \ingroup commands
 */
void CommandProcessor::setDefaultSettings() {
  tempControl.loadDefaultSettings();
  sendControlSettings(); // update script with new settings
  logInfo(INFO_DEFAULT_SETTINGS_LOADED);
}

/**
 * \brief Enable/disable the alarm buzzer.
 *
 * \ingroup commands
 */
void CommandProcessor::setAlarmState(bool enabled) { alarm_actuator.setActive(enabled); }

/**
 * \brief List devices that have been installed.
 *
 * Installed devices are devices that have been mapped to a control function.
 * \ingroup commands
 */
void CommandProcessor::listDevices() {
  DynamicJsonDocument doc(2048);
  deviceManager.listDevices(doc);
  piLink.sendJsonMessage('d', doc);
}

/**
 * \brief List all hardware devices.
 *
 * \ingroup commands
 */
void CommandProcessor::listHardware() {
  DynamicJsonDocument doc(2048);
  deviceManager.enumerateHardware(doc);
  piLink.sendJsonMessage('h', doc);
}

/**
 * \brief Reset the WiFi configuration
 *
 * \ingroup commands
 */
void CommandProcessor::resetWiFi() { WiFi.disconnect(true); }

/**
 * \brief Get info about the WiFi connection
 *
 * \ingroup commands
 */
void CommandProcessor::wifiInfo() {
  if (!Config::PiLink::useWifi) {
    commandNotImplemented('W', String("WiFi info not available when WiFi is disabled"));
    return;
  }

  DynamicJsonDocument doc(1024);
  wifi_connection_info(doc);
  piLink.sendJsonMessage('W', doc);
}

/**
 * \brief Toggle the state of the LCD backlight
 *
 * \ingroup commands
 */
void CommandProcessor::toggleBacklight() { ::toggleBacklight = !::toggleBacklight; }

/**
 * \brief Get what is currently displayed on the LCD
 *
 * \ingroup commands
 */
void CommandProcessor::getLcdContent() {
  DynamicJsonDocument doc(256);
  JsonArray rootArray = doc.to<JsonArray>();
  char stringBuffer[Config::Lcd::columns + 1];

  for (uint8_t i = 0; i < Config::Lcd::lines; i++) {
    display.getLine(i, stringBuffer);
    rootArray.add(stringBuffer);
  }
  piLink.sendJsonMessage('L', doc);
}

/**
 * \brief Send the current temperatures
 *
 * \ingroup commands
 */
void CommandProcessor::printTemperatures() {
  // print all temperatures with empty annotations
  piLink.printTemperatures(0, 0);
}

/**
 * \brief Send the current temperatures
 *
 * \ingroup commands
 */
void CommandProcessor::printRawTemperatures() {
  DynamicJsonDocument doc(1024);
  deviceManager.rawDeviceValues(doc);
  piLink.sendJsonMessage('R', doc);
}

/**
 * \brief Initialize EEPROM data
 *
 * \see EepromManager::initializeEeprom()
 */
void CommandProcessor::initEeprom() {
  if(eepromManager.initializeEeprom()) {
    logInfo(INFO_EEPROM_INITIALIZED);
    settingsManager.loadSettings();
  }
}

/**
 * \brief Print out the configured device names
 */
void CommandProcessor::printDeviceNames() {
  DynamicJsonDocument doc(256);
  DeviceNameManager::enumerateDeviceNames(doc);
  piLink.sendJsonMessage('N', doc);
}

/**
 * \brief Process incoming settings
 */
void CommandProcessor::processSettingsJson() {
  DynamicJsonDocument doc(256);
  piLink.receiveJsonMessage(doc);

  // Process
  JsonObject root = doc.as<JsonObject>();
  for (JsonPair kv : root) {
    SettingLoader::processSettingKeypair(kv);
  }

  // Save the settings
  eepromManager.storeTempConstantsAndSettings();

  // Inform the other end of the new state of affairs
  sendControlSettings();
  sendControlConstants();
}

/**
 * \brief Set device names
 *
 * @see DeviceNameManager::setDeviceName
 */
void CommandProcessor::setDeviceNames() {
  DynamicJsonDocument doc(256);
  piLink.receiveJsonMessage(doc);

  JsonObject root = doc.as<JsonObject>();
  for (JsonPair kv : root) {
    DeviceNameManager::setDeviceName(kv.key().c_str(), kv.value().as<const char *>());
  }

  // The probe names are used in the prometheus output, so changing the
  // name should invalidate the cache.
  if (Config::Prometheus::enable())
    promServer.invalidateCache();
}

/**
 * \brief Send control settings as JSON string
 */
void CommandProcessor::sendControlSettings() {
  DynamicJsonDocument doc(256);
  tempControl.getControlSettingsDoc(doc);
  piLink.sendJsonMessage('S', doc);
}

/**
 * \brief Send control constants as JSON string.
 * Might contain spaces between minus sign and number. Python will have to strip
 * these
 */
void CommandProcessor::sendControlConstants() {
  DynamicJsonDocument doc(1024);
  tempControl.getControlConstantsDoc(doc);
  piLink.sendJsonMessage('C', doc);
}

/**
 * \brief Send all control variables.
 *
 * Useful for debugging and choosing parameters
 */
void CommandProcessor::sendControlVariables() {
  DynamicJsonDocument doc(1024);
  tempControl.getControlVariablesDoc(doc);
  piLink.sendJsonMessage('V', doc);
}
