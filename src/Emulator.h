/*
 * Copyright 2012-2013 BrewPi/Elco Jacobs.
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

#include "Brewpi.h"

class MockSerial : public Stream {
public:
  void print(char c) {}
  void print(const char *c) {}
  void printNewLine() {}
  void println() {}
  int read() { return -1; }
  int available() { return -1; }
  void begin(unsigned long) {}
  size_t write(uint8_t w) { return 1; }
  int peek() { return -1; }
  void flush(){};
  operator bool() { return true; }
};

static MockSerial mockSerial;
