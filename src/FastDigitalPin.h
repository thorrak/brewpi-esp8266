/*
 * Copyright 2013 BrewPi/Elco Jacobs.
 * Copyright 2013 Matthew McGowan.
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

#include <driver/gpio.h>

// Arduino-compatible pin mode / level constants (for legacy call sites)
#ifndef INPUT
#define INPUT   0x00
#endif
#ifndef OUTPUT
#define OUTPUT  0x01
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x05
#endif
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW  0
#endif

inline void fastPinMode(int pin, int mode) {
	gpio_config_t io_conf = {};
	io_conf.pin_bit_mask = (1ULL << pin);
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	if (mode == OUTPUT) {
		io_conf.mode = GPIO_MODE_OUTPUT;
	} else if (mode == INPUT_PULLUP) {
		io_conf.mode = GPIO_MODE_INPUT;
		io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	} else { // INPUT
		io_conf.mode = GPIO_MODE_INPUT;
	}
	gpio_config(&io_conf);
}

inline void fastDigitalWrite(int pin, int value) {
	gpio_set_level((gpio_num_t)pin, value);
}

inline int fastDigitalRead(int pin) {
	return gpio_get_level((gpio_num_t)pin);
}

