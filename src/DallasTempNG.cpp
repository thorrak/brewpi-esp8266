#include "DallasTempNG.h"

typedef uint8_t ScratchPad[9];

// Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8


bool detectedReset(DallasTemperature &sensor, const uint8_t* scratchPad) {
	bool reset = (scratchPad[HIGH_ALARM_TEMP]==0);
	return reset;
}

bool initConnection(DallasTemperature &sensor, const uint8_t* deviceAddress) {
	ScratchPad scratchPad;
	
	if (!sensor.isConnected(deviceAddress, scratchPad)) {
		return false;
	}
		
	// assume the sensor has just been powered on. So this should only be called on initializtion, or
	// after a device was disconnected.			
	if (scratchPad[HIGH_ALARM_TEMP]) {		// conditional to avoid wear on eeprom. 			
		scratchPad[HIGH_ALARM_TEMP] = 0;
		sensor.writeScratchPad(deviceAddress, scratchPad);	
        sensor.saveScratchPad(deviceAddress);  // save to eeprom
		
		// check if the write was successful (HIGH_ALARAM_TEMP == 0)
		if (!sensor.isConnected(deviceAddress, scratchPad) || !detectedReset(sensor, scratchPad))
			return false;		
	}

	scratchPad[HIGH_ALARM_TEMP]=1;
	sensor.writeScratchPad(deviceAddress, scratchPad);	// don't save to eeprom, so that it reverts to 0 on reset
	// from this point on, if we read a scratchpad with a 0 value in HIGH_ALARM (detectedReset() returns true)
	// it means the device has reset or the previous write of the scratchpad above was unsuccessful.
	// Either way, initConnection() should be called again
	return true;
}


int16_t getTempRaw(DallasTemperature &sensor, const uint8_t* deviceAddress)
{
    ScratchPad scratchPad;
    if (sensor.isConnected(deviceAddress, scratchPad) && !detectedReset(sensor, scratchPad)) 
        return sensor.getTemp(deviceAddress) >> 3;	
    return DEVICE_DISCONNECTED_RAW;		// use a value that the sensor could not ordinarily measure
}
