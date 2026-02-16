/* 
 * File:   SensorArduinoPin.h
 * Author: mat
 *
 * Created on 22 August 2013, 19:36
 */

#pragma once

#include "Brewpi.h"
#include "FastDigitalPin.h"
#include "Pins.h"
#include <driver/gpio.h>

/* A SwitchSensor whose state is provided by a hardware pin.
  By using a template, the compiler can inline and optimize the call to digitalRead to a single instruction.
*/
template<uint8_t pin, bool invert, bool internalPullup>
class DigitalConstantPinSensor : public SwitchSensor
{
	public:
	DigitalConstantPinSensor() {
		gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
		if (internalPullup) {
			gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY);
		}
	}

	virtual bool sense() {
		return gpio_get_level((gpio_num_t)pin) ^ invert;
	}
};

class DigitalPinSensor : public SwitchSensor
{
private:
	bool invert;
	uint8_t pin;

public:

	DigitalPinSensor(uint8_t pin, bool invert)
	{
		gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
		if (USE_INTERNAL_PULL_UP_RESISTORS) {
			gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY);
		}
		this->invert = invert;
		this->pin = pin;
	}

	virtual bool sense() {
		return gpio_get_level((gpio_num_t)pin) ^ invert;
	}
};

