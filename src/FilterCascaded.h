/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
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

#include "TemperatureFormats.h"
#include "FilterFixed.h"


/**
 * \brief Implements a filter that consists of multiple second order sections.
 */
class CascadedFilter {
	public:
  /**
   * \brief Number of filter sections to use
   *
   * Three filter sections gives excellent filtering, without adding too
   * much delay.  For 3 sections the stop band attenuation is 3x the single
   * section attenuation in dB.  The delay is also tripled.
   */
  constexpr static int numFilterSections = 3;

  /**
   * \brief Filter sections
   */
	FixedFilter sections[numFilterSections];

	public:
	CascadedFilter();
	~CascadedFilter() {}
	void init(temperature val);
	void setCoefficients(uint8_t bValue);
	temperature add(temperature val);
	temperature_precise addDoublePrecision(temperature_precise val);
	temperature readInput(void);

  /**
   * \brief Read the output
   */
	temperature readOutput(void){
		return sections[numFilterSections - 1].readOutput(); // return output of last section
	}
	temperature_precise readOutputDoublePrecision(void);
	temperature_precise readPrevOutputDoublePrecision(void);

  /**
   * \brief Do positive peak detection
   */
	temperature detectPosPeak(void){
		return sections[numFilterSections - 1].detectPosPeak(); // detect peaks in last section
	}

  /**
   * \brief Do negative peak detection
   */
	temperature detectNegPeak(void){
		return sections[numFilterSections - 1].detectNegPeak(); // detect peaks in last section
	}
};
