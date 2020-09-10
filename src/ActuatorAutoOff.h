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

#include "Actuator.h"
#include "Ticks.h"

/**
 * An Actuator wrapper that automaically turns the wrapped Actuator off after
 * an elapsed timeout
 */
class AutoOffActuator : public Actuator {
	
public:
  /**
   * Constructor.
   *
   * @param timeout - Number of seconds to stay on after activated
   * @param target - Acutuator to manage
   */
	AutoOffActuator(uint16_t timeout, Actuator* target) {
		this->timeout = timeout;
		this->target = target;
	}

  /**
   * \brief Set the actuator state
   *
   * @param active - New state
   * @see Actuator::setActive()
   */
	void setActive(bool active)
	{
		this->active = active;
		target->setActive(active);
		if (active)
			lastActiveTime = ticks.seconds();
	}

  /**
   * \brief Get actuator current state
   */
	bool isActive() {
		return active; //target->isActive(); - this takes 20 bytes more
	}

  /**
   * \brief Update the actuator state.
   * If the timeout period has elapsed, sets the state to false.
   */
	void update() {
		if (ticks.timeSince(lastActiveTime)>=timeout)
			setActive(false);
	}

private:
	uint16_t lastActiveTime; //!< Tick count when the actuator was last made active.
	uint16_t timeout; //!< Length of time that the actuator should be enabled before turning off.
	Actuator* target; //!< Target Actuator that is being controlled.

  /**
   * \brief Current actuator state.
   * This effectively duplicates the state that is in the target Actuator, but
   * by keeping value here we avoid call overhead to get that value.
   */
	bool active;
};
