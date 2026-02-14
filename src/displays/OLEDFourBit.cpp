/*
 * Copyright 2012 BrewPi/Elco Jacobs.
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

#if BREWPI_LCD
#include "OLEDFourBit.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>


void OLEDFourBit::init(uint8_t rs, uint8_t rw, uint8_t enable,
					uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
	_rs_pin = rs;
	_rw_pin = rw;
	_enable_pin = enable;
	_busy_pin = d7;
  
	_data_pins[0] = d4;
	_data_pins[1] = d5;
	_data_pins[2] = d6;
	_data_pins[3] = d7; 


	gpio_set_direction((gpio_num_t)_rs_pin, GPIO_MODE_OUTPUT);
	gpio_set_direction((gpio_num_t)_rw_pin, GPIO_MODE_OUTPUT);
	gpio_set_direction((gpio_num_t)_enable_pin, GPIO_MODE_OUTPUT);

	_displayfunction = LCD_FUNCTIONSET | LCD_4BITMODE;
   
}

void OLEDFourBit::begin(uint8_t cols, uint8_t lines) {
	_numlines = lines;
	_currline = 0;
	_currpos = 0;
  
	gpio_set_direction((gpio_num_t)_rs_pin, GPIO_MODE_OUTPUT);
	gpio_set_direction((gpio_num_t)_rw_pin, GPIO_MODE_OUTPUT);
	gpio_set_direction((gpio_num_t)_enable_pin, GPIO_MODE_OUTPUT);

	// Now we pull both RS and R/W low to begin commands
	gpio_set_level((gpio_num_t)_rs_pin, 0);
	gpio_set_level((gpio_num_t)_enable_pin, 0);
	gpio_set_level((gpio_num_t)_rw_pin, 0);

	for (int i = 0; i < 4; i++) {
		gpio_set_direction((gpio_num_t)_data_pins[i], GPIO_MODE_OUTPUT);
		gpio_set_level((gpio_num_t)_data_pins[i], 0);
	}
	
	// SEE PAGE 20 of NHD-0420DZW-AY5 
	delayMicroseconds(50000); // wait 50 ms just to be sure tha the lcd is initialized
 
	delayMicroseconds(32000);
	write4bits(0x03);
	delayMicroseconds(32000);
	write4bits(0x03);
	delayMicroseconds(32000);
	write4bits(0x03);
	  
	delayMicroseconds(32000);
	write4bits(0x02);
	delayMicroseconds(10000);
	write4bits(0x02);
	delayMicroseconds(10000);
	write4bits(0x08);
   
	waitBusy();
   
	noDisplay();	// Display off
		
	clear();	// display clear
	
	// Entry Mode Set:
	leftToRight();
	noAutoscroll();
	
	home();
	
	noCursor();
	display();
}

/********** high level commands, for the user! */
void OLEDFourBit::clear()
{
	command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
	
	for(uint8_t i = 0; i<4; i++){
		for(uint8_t j = 0; j<20; j++){
			content[i][j]=' '; // initialize on all spaces
		}
		content[i][20]='\0'; // NULL terminate string
	}	
}

void OLEDFourBit::home()
{
	command(LCD_RETURNHOME);  // set cursor position to zero
	_currline = 0;
	_currpos = 0;
}

void OLEDFourBit::setCursor(uint8_t col, uint8_t row)
{
	uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row >= _numlines ) {
		row = 0;  //write to first line if out off bounds
	}
	_currline = row;
	_currpos = col;
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void OLEDFourBit::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void OLEDFourBit::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void OLEDFourBit::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void OLEDFourBit::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void OLEDFourBit::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void OLEDFourBit::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void OLEDFourBit::scrollDisplayLeft() {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void OLEDFourBit::scrollDisplayRight() {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void OLEDFourBit::leftToRight() {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void OLEDFourBit::rightToLeft() {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void OLEDFourBit::autoscroll() {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void OLEDFourBit::noAutoscroll() {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void OLEDFourBit::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}

/*********** mid level commands, for sending data/cmds */

inline void OLEDFourBit::command(uint8_t value) {
	send(value, LOW);
	waitBusy();
}

inline size_t OLEDFourBit::write(uint8_t value) {
	send(value, HIGH);
	content[_currline][_currpos] = value;
	_currpos++;
	waitBusy();
	return 1;
}

/************ low level data pushing commands **********/

// write either command or data
void OLEDFourBit::send(uint8_t value, uint8_t mode) {
	gpio_set_level((gpio_num_t)_rs_pin, mode);
	gpio_set_direction((gpio_num_t)_rw_pin, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)_rw_pin, 0);

	write4bits(value>>4);
	write4bits(value);
}

void OLEDFourBit::pulseEnable() {
	gpio_set_level((gpio_num_t)_enable_pin, 1);
	delayMicroseconds(100); // enable pulse must be >450ns
	gpio_set_level((gpio_num_t)_enable_pin, 0);
}

void OLEDFourBit::write4bits(uint8_t value) {
	for (int i = 0; i < 4; i++) {
		gpio_set_direction((gpio_num_t)_data_pins[i], GPIO_MODE_OUTPUT);
		gpio_set_level((gpio_num_t)_data_pins[i], (value >> i) & 0x01);
	}
	delayMicroseconds(100);
	pulseEnable();
}

void OLEDFourBit::waitBusy() {
	uint8_t busy = 1;
	gpio_set_direction((gpio_num_t)_busy_pin, GPIO_MODE_INPUT);
	gpio_set_level((gpio_num_t)_rs_pin, 0);
	gpio_set_level((gpio_num_t)_rw_pin, 1);
	uint8_t tries = 0;
	do{
		gpio_set_level((gpio_num_t)_enable_pin, 0);
		gpio_set_level((gpio_num_t)_enable_pin, 1);
		delayMicroseconds(10);
		busy = gpio_get_level((gpio_num_t)_busy_pin);
		gpio_set_level((gpio_num_t)_enable_pin, 0);
		pulseEnable(); // get remaining 4 bits, which are not used.
		tries++;
		if(tries>200){
			break;
		}
	}while(busy);

	gpio_set_direction((gpio_num_t)_busy_pin, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)_rw_pin, 0);
}

char OLEDFourBit::readChar(){
	char value=0x00;
	for (int i = 0; i < 4; i++) {
		gpio_set_direction((gpio_num_t)_data_pins[i], GPIO_MODE_INPUT);
	}
	gpio_set_level((gpio_num_t)_rs_pin, 1);
	gpio_set_level((gpio_num_t)_rw_pin, 1);
	pulseEnable();
	delayMicroseconds(600);
	for (int i = 0; i < 4; i++) {
		value = value | (gpio_get_level((gpio_num_t)_data_pins[i]) << (i+4));
	}
	pulseEnable();
	delayMicroseconds(600);
	for (int i = 0; i < 4; i++) {
		value = value | (gpio_get_level((gpio_num_t)_data_pins[i]) << (i));
	}
	return value;
}

void OLEDFourBit::getLine(uint8_t lineNumber, char * buffer){
	const char* src = content[lineNumber];
	for(uint8_t i =0;i<20;i++){
		char c = src[i];
		buffer[i] = (c == 0b11011111) ? 0xB0 : c;
	}
	buffer[20] = '\0'; // NULL terminate string
}	

// Read the content from the display and store it in the local string buffer.
// Buffer should always stay up to date, so this function is not really needed.
void OLEDFourBit::readContent(){
	setCursor(0,0);
	for(uint8_t i =0;i<20;i++){
		content[0][i] = readChar();
	}
	for(uint8_t i =0;i<20;i++){
		content[2][i] = readChar();
	}
	setCursor(0,1);
	for(uint8_t i =0;i<20;i++){
		content[1][i] = readChar();
	}
	for(uint8_t i =0;i<20;i++){
		content[3][i] = readChar();
	}
}

void OLEDFourBit::printSpacesToRestOfLine(){
	while(_currpos < 20){
		print(' ');
	}
}

#endif
