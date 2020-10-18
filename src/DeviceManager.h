/*
 * Copyright 2013 Matthew McGowan
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

#include "Actuator.h"
#include "Sensor.h"
#include "TempSensor.h"
#include "OneWireDevices.h"
#include "Pins.h"
#include "EepromStructs.h"


#ifdef ARDUINO
#include "DallasTemperature.h"	// for DeviceAddress
#endif

/**
 * \defgroup hardware Hardware
 * \brief Interfacing with hardware devices
 *
 * A user has freedom to connect various devices to the arduino, either via
 * extending the oneWire bus, or by assigning to specific pins, e.g. actuators,
 * switch sensors.  Rather than make this compile-time, the configuration is
 * stored at runtime.  Also, the availability of various sensors will change.
 * E.g. it's possible to have a fridge constant mode without a beer sensor.
 *
 * Since the data has to be persisted to EEPROM, references to the actual uses
 * of the devices have to be encoded.  This is the function of the deviceID.
 *
 * \addtogroup hardware
 * @{
 */

class DeviceConfig;

/**
 * \brief Device slot identifier
 */
typedef int8_t device_slot_t;

/**
 * \brief Check if slot is valid
 */
inline bool isDefinedSlot(device_slot_t s) { return s>=0; }

/**
 * \brief Maximum number of device slots
 */
const device_slot_t MAX_DEVICE_SLOT = 16;		// exclusive

/**
 * \brief An invalid device slot
 */
const device_slot_t INVALID_SLOT = -1;


/**
 * \brief Describes where the device is most closely associated.
 */
enum DeviceOwner {
	DEVICE_OWNER_NONE=0, //!< No owner
	DEVICE_OWNER_CHAMBER=1, //!< Chamber/Fridge
	DEVICE_OWNER_BEER=2 //!< Beer
};

/**
 * \brief Hardware device type
 */
enum DeviceType {
	DEVICETYPE_NONE = 0, //!< Unconfigured/unknown
	DEVICETYPE_TEMP_SENSOR = 1,		//!< BasicTempSensor - OneWire
	DEVICETYPE_SWITCH_SENSOR = 2,		//!< SwitchSensor - direct pin and onewire are supported
	DEVICETYPE_SWITCH_ACTUATOR = 3	//!< Actuator - both direct pin and onewire are supported
};


/**
 * \brief Check if device is assignable.
 *
 * @param hardware - Hardware device to check
 */
inline bool isAssignable(DeviceType type, DeviceHardware hardware)
{
	return (hardware==DEVICE_HARDWARE_PIN && (type==DEVICETYPE_SWITCH_ACTUATOR || type==DEVICETYPE_SWITCH_SENSOR))
#if BREWPI_DS2413
	|| (hardware==DEVICE_HARDWARE_ONEWIRE_2413 && (type==DEVICETYPE_SWITCH_ACTUATOR || (DS2413_SUPPORT_SENSE && type==DEVICETYPE_SWITCH_SENSOR)))
#endif
	|| (hardware==DEVICE_HARDWARE_ONEWIRE_TEMP && type==DEVICETYPE_TEMP_SENSOR)
	|| (hardware==DEVICE_HARDWARE_NONE && type==DEVICETYPE_NONE);
}


/**
 * \brief Check if device is a OneWire device.
 *
 * @param hardware - Hardware device to check
 * @returns `true` if device is OneWire, `false` otherwise
 */
inline bool isOneWire(DeviceHardware hardware) {
	return
#if BREWPI_DS2413
	hardware==DEVICE_HARDWARE_ONEWIRE_2413 ||
#endif
	hardware==DEVICE_HARDWARE_ONEWIRE_TEMP;
}

/**
 * \brief Check if device is a Digital Pin device.
 *
 * @param hardware - Hardware device to check
 * @returns `true` if device is a digital pin, `false` otherwise
 */
inline bool isDigitalPin(DeviceHardware hardware) {
	return hardware==DEVICE_HARDWARE_PIN;
}

extern DeviceType deviceType(DeviceFunction id);


/**
 * \brief Determines where devices belongs, based on its function.
 *
 * @param id - Device Function
 */
inline DeviceOwner deviceOwner(DeviceFunction id) {
	return id==0 ? DEVICE_OWNER_NONE : id>=DEVICE_BEER_FIRST ? DEVICE_OWNER_BEER : DEVICE_OWNER_CHAMBER;
}


/**
 * \brief Provides a single alternative value for a given definition point in a device.
 */
struct DeviceAlternatives {
	enum AlternativeType {
		DA_PIN,
    DA_ADDRESS,
    DA_PIO,
    DA_INVERT,
    DA_BOOLVALUE
	};
	AlternativeType type;
	union {
		uint8_t pinNr;					// type == DA_PIN
		uint8_t pio;					// type == DA_PIO
		DeviceAddress address;			// type == DA_ADDRESS
		bool invert;					// type == DA_INVERT
		bool boolValue;					// type == DA_BOOLVALUE
	};

};


/**
 * \brief Function pointer for enumerate device callback
 *
 * @see DeviceManager::enumerateOneWireDevices
 * @see DeviceManager::enumeratePinDevices
 */
typedef void (*EnumDevicesCallback)(DeviceConfig*, void* pv, JsonDocument* doc);

struct DeviceOutput
{
	device_slot_t	slot;
	char value[10];
};

/**
 * \brief Control structure used by methods to limit/filter processed devices.
 *
 * Various enumerate methods use the DeviceDisplay structure to control which devices are processed.
 * @see DeviceManager::enumDevice
 * @see DeviceManager::UpdateDeviceState
 */
struct DeviceDisplay {
	int8_t id;		//!< -1 for all devices, >=0 for specific device
	int8_t value;	//!< set value
	int8_t write;	//!< write value
	int8_t empty;	//!< show unused devices when id==-1, default is 0
};


/**
 * \brief Hardware device definition.
 */
struct DeviceDefinition {
	int8_t id;
	int8_t chamber;
	int8_t beer;
	int8_t deviceFunction;
	int8_t deviceHardware;
	int8_t pinNr;
	int8_t invert;
	int8_t pio;
	int8_t deactivate;
	int8_t calibrationAdjust;
	DeviceAddress address;

	/**
	 * Lists the first letter of the key name for each attribute.
	 */
	static const char ORDER[12];
};


/**
 * \brief Hardware filter definition
 */
struct EnumerateHardware
{
	int8_t hardware;	//<! Restrict the types of devices requested
	int8_t pin;				//<! Pin to search
	int8_t values;		//<! Fetch values for the devices.
	int8_t unused;		//<! 0 don't care about unused state, 1 unused only.
	int8_t function;	//<! Restrict to devices that can be used with this function
};

void HandleDeviceDisplay(const char* key, const char* value, void* pv);

/**
 * Reads or writes a value to a device.
 */
void UpdateDeviceState(DeviceDisplay& dd, DeviceConfig& dc, char* val);

class OneWire;


/**
 * \brief Manage the different hardware devices that can be attached.
 *
 * Gives a common interface for interacting with different hardware types.
 */
class DeviceManager
{
public:

	bool isDefaultTempSensor(BasicTempSensor* sensor);

	int8_t enumerateActuatorPins(uint8_t offset)
	{
#if BREWPI_ACTUATOR_PINS && defined(ARDUINO)
#if BREWPI_STATIC_CONFIG<=BREWPI_SHIELD_REV_A
		switch (offset) {
			case 0: return heatingPin;
			case 1: return coolingPin;
			default:
				return -1;
		}
#elif BREWPI_STATIC_CONFIG>=BREWPI_SHIELD_REV_C
		switch (offset) {
			case 0: return actuatorPin1;
			case 1: return actuatorPin2;
			case 2: return actuatorPin3;
			case 3: return actuatorPin4;
			default: return -1;
		}
#endif			
#endif
		return -1;
	}

	int8_t enumerateSensorPins(uint8_t offset) {
#if BREWPI_SENSOR_PINS && defined(ARDUINO)
		if (offset==0)
			return doorPin;
#endif
		return -1;
	}

	/**
   * Enumerates the OneWire bus pins.
	 */
	int8_t enumOneWirePins(uint8_t offset) {
#ifdef ARDUINO
#ifdef oneWirePin
		if (offset == 0)
			return oneWirePin;
#elif defined(beerSensorPin) && defined(fridgeSensorPin)
		if (offset==0)
			return beerSensorPin;
		if (offset==1)
			return fridgeSensorPin;
#endif
#endif
		return -1;
	}

	static void setupUnconfiguredDevices();

	/**
	 * \brief Determines if the given device config is complete.
	 */
	static bool firstUndefinedAlternative(DeviceConfig& config, DeviceAlternatives& alternatives);


	static void installDevice(DeviceConfig& config);

	static void uninstallDevice(DeviceConfig& config);

	static void parseDeviceDefinition();
	static void serializeJsonDevice(JsonDocument&, device_slot_t slot, DeviceConfig& config, const char* value);

	static bool allDevices(DeviceConfig& config, uint8_t deviceIndex);

	static bool isDeviceValid(DeviceConfig& config, DeviceConfig& original, int8_t deviceIndex);

	static void enumerateHardware(JsonDocument& doc);

	static bool enumDevice(DeviceDisplay& dd, DeviceConfig& dc, uint8_t idx);

	static void listDevices(JsonDocument& doc);
  static void rawDeviceValues(JsonDocument& doc);

private:
	static void enumerateOneWireDevices(EnumerateHardware& h, EnumDevicesCallback callback, DeviceOutput& output, JsonDocument* doc);
	static void enumeratePinDevices(EnumerateHardware& h, EnumDevicesCallback callback, DeviceOutput& output, JsonDocument* doc);
	static void outputEnumeratedDevices(DeviceConfig* config, void* pv, JsonDocument* doc);
	static void handleEnumeratedDevice(DeviceConfig& config, EnumerateHardware& h, EnumDevicesCallback callback, DeviceOutput& out, JsonDocument* doc);
	static void readTempSensorValue(DeviceConfig::Hardware hw, char* out);
	static void outputRawDeviceValue(DeviceConfig* config, void* pv, JsonDocument* doc);

  static void readJsonIntoDeviceDef(DeviceDefinition&);
  static void readJsonIntoDeviceDisplay(DeviceDisplay&);
  static void readJsonIntoHardwareSpec(EnumerateHardware&);

	static void* createDevice(DeviceConfig& config, DeviceType dc);
	static void* createOneWireGPIO(DeviceConfig& config, DeviceType dt);

	static OneWire* oneWireBus(uint8_t pin);

#ifdef ARDUINO

// There is no reason to separate the OneWire busses - if we have a single bus, use it.
#ifdef oneWirePin
	static OneWire primaryOneWireBus;
#else
	static OneWire beerSensorBus;
	static OneWire fridgeSensorBus;
#endif

#endif
};


/**
 * A global instance of DeviceManager
 */
extern DeviceManager deviceManager;
/** @} */
