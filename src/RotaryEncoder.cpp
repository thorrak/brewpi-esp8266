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
#include "RotaryEncoder.h"

#include "Pins.h"
#include <limits.h>
#include "Ticks.h"
#include "Display.h"
#include "Brewpi.h"
#include "TempControl.h"

RotaryEncoder rotaryEncoder;

int16_t RotaryEncoder::maximum;
int16_t RotaryEncoder::minimum;
volatile int16_t RotaryEncoder::steps;
volatile bool RotaryEncoder::pushFlag;

#if BREWPI_STATIC_CONFIG!=BREWPI_SHIELD_DIY
	#if rotarySwitchPin != 7
		#error Review interrupt vectors when not using pin 7 for menu push
	#endif
	#if rotaryAPin != 8
		#error Review interrupt vectors when not using pin 8 for menu right
	#endif
	#if rotaryBPin != 9
		#error Review interrupt vectors when not using pin 9 for menu left
	#endif
#else
	#if rotarySwitchPin != 0
	#error Review interrupt vectors when not using pin 0 for menu push
	#endif
	#if rotaryAPin != 2
	#error Review interrupt vectors when not using pin 2 for menu right
	#endif
	#if rotaryBPin != 1
	#error Review interrupt vectors when not using pin 1 for menu left
	#endif
#endif

#if BREWPI_ROTARY_ENCODER
#if BREWPI_BOARD!=BREWPI_BOARD_LEONARDO && BREWPI_BOARD!=BREWPI_BOARD_STANDARD
	#error Rotary encoder code is not compatible with boards other than leonardo or uno yet.
#endif
#endif

// Implementation based on work of Ben Buxton:

/* Rotary encoder handler for arduino. v1.1
 *
 * Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3.
 * Contact: bb@cactii.net
 *
 * A typical mechanical rotary encoder emits a two bit gray code
 * on 3 output pins. Every step in the output (often accompanied
 * by a physical 'click') generates a specific sequence of output
 * codes on the pins.
 *
 * There are 3 pins used for the rotary encoding - one common and
 * two 'bit' pins.
 *
 * The following is the typical sequence of code on the output when
 * moving from one step to the next:
 *
 *   Position   Bit1   Bit2
 *   ----------------------
 *     Step1     0      0
 *      1/4      1      0
 *      1/2      1      1
 *      3/4      0      1
 *     Step2     0      0
 *
 * From this table, we can see that when moving from one 'click' to
 * the next, there are 4 changes in the output code.
 *
 * - From an initial 0 - 0, Bit1 goes high, Bit0 stays low.
 * - Then both bits are high, halfway through the step.
 * - Then Bit1 goes low, but Bit2 stays high.
 * - Finally at the end of the step, both bits return to 0.
 *
 * Detecting the direction is easy - the table simply goes in the other
 * direction (read up instead of down).
 *
 * To decode this, we use a simple state machine. Every time the output
 * code changes, it follows state, until finally a full steps worth of
 * code is received (in the correct order). At the final 0-0, it returns
 * a value indicating a step in one direction or the other.
 *
 * It's also possible to use 'half-step' mode. This just emits an event
 * at both the 0-0 and 1-1 positions. This might be useful for some
 * encoders where you want to detect all positions.
 *
 * If an invalid state happens (for example we go from '0-1' straight
 * to '1-0'), the state machine resets to the start until 0-0 and the
 * next valid codes occur.
 *
 * The biggest advantage of using a state machine over other algorithms
 * is that this has inherent debounce built in. Other algorithms emit spurious
 * output with switch bounce, but this one will simply flip between
 * sub-states until the bounce settles, then continue along the state
 * machine.
 * A side effect of debounce is that fast rotations can cause steps to
 * be skipped. By not requiring debounce, fast rotations can be accurately
 * measured.
 * Another advantage is the ability to properly handle bad state, such
 * as due to EMI, etc.
 * It is also a lot simpler than others - a static state table and less
 * than 10 lines of logic.
 */

/*
 * The below state table has, for each state (row), the new state
 * to set based on the next encoder output. From left to right in,
 * the table, the encoder outputs are 00, 01, 10, 11, and the value
 * in that position is the new state to set.
 */

#define R_START 0x0
// #define HALF_STEP

// Use the half-step state table (emits a code at 00 and 11)
#define HS_R_CCW_BEGIN 0x1
#define HS_R_CW_BEGIN 0x2
#define HS_R_START_M 0x3
#define HS_R_CW_BEGIN_M 0x4
#define HS_R_CCW_BEGIN_M 0x5
const uint8_t PROGMEM hs_ttable[7][4] = {
	// R_START (00)
	{HS_R_START_M,            HS_R_CW_BEGIN,     HS_R_CCW_BEGIN,  R_START},
	// HS_R_CCW_BEGIN
	{HS_R_START_M | DIR_CCW, R_START,        HS_R_CCW_BEGIN,  R_START},
	// HS_R_CW_BEGIN
	{HS_R_START_M | DIR_CW,  HS_R_CW_BEGIN,     R_START,      R_START},
	// HS_R_START_M (11)
	{HS_R_START_M,            HS_R_CCW_BEGIN_M,  HS_R_CW_BEGIN_M, R_START},
	// HS_R_CW_BEGIN_M
	{HS_R_START_M,            HS_R_START_M,      HS_R_CW_BEGIN_M, R_START | DIR_CW},
	// HS_R_CCW_BEGIN_M
	{HS_R_START_M,            HS_R_CCW_BEGIN_M,  HS_R_START_M,    R_START | DIR_CCW},
	{R_START, R_START, R_START, R_START}
};

// Use the full-step state table (emits a code at 00 only)
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const uint8_t PROGMEM ttable[7][4] = {
	// R_START
	{R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
	// R_CW_FINAL
	{R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
	// R_CW_BEGIN
	{R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
	// R_CW_NEXT
	{R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
	// R_CCW_BEGIN
	{R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
	// R_CCW_FINAL
	{R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
	// R_CCW_NEXT
	{R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

#if BREWPI_ROTARY_ENCODER
#include "util/atomic.h"
#include "FastDigitalPin.h"


#if BREWPI_STATIC_CONFIG==BREWPI_SHIELD_DIY
ISR(INT2_vect) {
	rotaryEncoder.setPushed();
}
ISR(INT3_vect) {
	rotaryEncoder.process();
}
ISR(INT1_vect) {
	rotaryEncoder.process();
}
#elif BREWPI_BOARD == BREWPI_BOARD_LEONARDO
ISR(INT6_vect){
	rotaryEncoder.setPushed();
}
ISR(PCINT0_vect){
	rotaryEncoder.process();
}
#elif BREWPI_BOARD == BREWPI_BOARD_STANDARD
ISR(PCINT2_vect){
	if(!bitRead(PIND,7)){
		// high to low transition
		rotaryEncoder.setPushed();
	}
}
ISR(PCINT0_vect){
	rotaryEncoder.process();
}
#else
	#error board/processor not supported by rotary encoder code. Disable or fix the rotary encoder.
#endif


void RotaryEncoder::process(void){
	static uint8_t state=R_START;
	// Grab state of input pins.
	#if BREWPI_STATIC_CONFIG == BREWPI_SHIELD_DIY
	uint8_t currPinA = !bitRead(PIND,2);
	uint8_t currPinB = !bitRead(PIND,3);
	#elif BREWPI_BOARD == BREWPI_BOARD_LEONARDO
	uint8_t currPinA = !bitRead(PINB,4);
	uint8_t currPinB = !bitRead(PINB,5);
	#elif BREWPI_BOARD == BREWPI_BOARD_STANDARD
	uint8_t currPinA = !bitRead(PINB,0);
	uint8_t currPinB = !bitRead(PINB,1);
	#endif
	
	unsigned char pinstate = (currPinB << 1) | currPinA;

	// Determine new state from the pins and state table.
	if(tempControl.cc.rotaryHalfSteps){
		state = pgm_read_byte(&(hs_ttable[state & 0xf][pinstate]));	
	}
	else{
		state = pgm_read_byte(&(ttable[state & 0xf][pinstate]));	
	}
	
	// Get emit bits, ie the generated event.
	
	uint8_t dir = state & 0x30;
	
	if(dir){
		int16_t s = steps;	// steps is volatile - save a copy here to avoid multiple fetches
		s = (dir==DIR_CW) ? s+1 : s-1;
		if (s > maximum)	
			s = minimum;
		else if (s < minimum)
			s = maximum;	
		steps = s;
		display.resetBacklightTimer();
	}	
}
#endif  // BREWPI_ROTARY_ENCODER

void RotaryEncoder::setPushed(void){
	pushFlag = true;
	display.resetBacklightTimer();
}


void RotaryEncoder::init(void){
#if BREWPI_ROTARY_ENCODER
	#define BREWPI_INPUT_PULLUP (USE_INTERNAL_PULL_UP_RESISTORS ? INPUT_PULLUP : INPUT)
	fastPinMode(rotaryAPin, BREWPI_INPUT_PULLUP);
	fastPinMode(rotaryBPin, BREWPI_INPUT_PULLUP);
	fastPinMode(rotarySwitchPin, BREWPI_INPUT_PULLUP);
	
	#if BREWPI_STATIC_CONFIG==BREWPI_SHIELD_DIY
		EICRA |= (1<<ISC21) | (1<<ISC10) | (1<<ISC30);; // any logical change for encoder pins, falling edge for switch
		EIMSK |= (1<<INT2) | (1<<INT1) | (1<<INT3); // enable interrupts for each pin	
	#elif BREWPI_BOARD == BREWPI_BOARD_LEONARDO
		// falling edge interrupt for switch on INT6
		EICRB |= (1<<ISC61) | (0<<ISC60);
		// enable interrupt for INT6
		EIMSK |= (1<<INT6);
		// enable pin change interrupts
		PCICR |= (1<<PCIE0);
		// enable pin change interrupt on Arduino pin 8 and 9
		PCMSK0 |= (1<<PCINT5) | (1<<PCINT4);
	#elif BREWPI_BOARD == BREWPI_BOARD_STANDARD
		// enable PCINT0 (PCINT0 and PCINT1 pin) and PCINT2 vector (PCINT23 pin)
		PCICR |= (1<<PCIE2) | (1<<PCIE0);
		// enable mask bits for PCINT0 and PCINT1
		PCMSK0 |= (1<<PCINT0) | (1<<PCINT1);
		// enable mask bit for PCINT23
		PCMSK2 |= (1<<PCINT23);
	#endif
#endif	
}


void RotaryEncoder::setRange(int16_t start, int16_t minVal, int16_t maxVal){
#if BREWPI_ROTARY_ENCODER    
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		// this part cannot be interrupted
		// Multiply by two to convert to half steps
		steps = start;
		minimum = minVal;
		maximum = maxVal; // +1 to make sure that one step is still two half steps at overflow
	}		
#endif        
}

bool RotaryEncoder::changed(void){
	// returns one if the value changed since the last call of changed.
	static int16_t prevValue = 0;
	int16_t r = read();
	if(r != prevValue){
		prevValue = r;
		return 1;
	}
	if(pushFlag == true){
		return 1;
	}
	return 0;
}

int16_t RotaryEncoder::read(void){
#if BREWPI_ROTARY_ENCODER
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		return steps;		
	}
#endif
	return 0;		
}
