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

#include "Brewpi.h"
#include "FilterFixed.h"
#include "FilterCascaded.h"
#include <stdlib.h>
#include <limits.h>
#include "TemperatureFormats.h"

CascadedFilter::CascadedFilter() {
  // default to a b value of 2
  setCoefficients(2);
}

/**
 * \brief Set all secition coefficients to a given value
 *
 * @param bValue new coefficient value
 */
void CascadedFilter::setCoefficients(uint8_t bValue){
	for(uint8_t i=0; i<numFilterSections; i++){
		sections[i].setCoefficients(bValue);
	}
}


/**
 * \brief Adds a value and returns the most recent filter output
 */
temperature CascadedFilter::add(temperature val){
	temperature_precise valDoublePrecision = tempRegularToPrecise(val);
	valDoublePrecision = addDoublePrecision(valDoublePrecision);
	// return output, shifted back to single precision
	return tempPreciseToRegular(valDoublePrecision);
}


/**
 * \brief Add a precise temperature value to the filter
 */
temperature_precise CascadedFilter::addDoublePrecision(temperature_precise val){
	temperature_precise input = val;
	// input is input for next section, which is the output of the previous section
	for(uint8_t i=0; i<numFilterSections; i++){
		input = sections[i].addDoublePrecision(input);
	}
	return input;
}


/**
 * \brief Get the most recent filter input
 */
temperature CascadedFilter::readInput(void){
	return sections[0].readInput(); // return input of first section
}

/**
 * \brief Get the output value
 */
temperature_precise CascadedFilter::readOutputDoublePrecision(void){
	return sections[numFilterSections - 1].readOutputDoublePrecision(); // return output of last section
}

/**
 * \brief Get the previous output value
 */
temperature_precise CascadedFilter::readPrevOutputDoublePrecision(void){
	return sections[numFilterSections - 1].readPrevOutputDoublePrecision(); // return previous output of last section
}


/**
 * \brief Initialize filter.
 *
 * @param val - Temperature to seed the filter with.
 */
void CascadedFilter::init(temperature val){
	for(uint8_t i=0; i<numFilterSections; i++){
		sections[i].init(val);
	}
}

