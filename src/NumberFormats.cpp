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

#include "NumberFormats.h"

/**
 * \brief Parse string into 8-bit bytes
 */
void parseBytes(uint8_t *data, const char *s, uint8_t len) {
  char c;
  while ((c = *s++)) {
    uint8_t d = (c >= 'A' ? c - 'A' + 10 : c - '0') << 4;
    c = *s++;
    d |= (c >= 'A' ? c - 'A' + 10 : c - '0');
    *data++ = d;
  }
}

/**
 * \brief Render 8-bit data as hex
 */
void printBytes(const uint8_t *data, uint8_t len, char *buf) {
  for (int i = 0; i < len; i++) {
    uint8_t b = (data[i] >> 4) & 0x0f;
    *buf++ = (b > 9 ? b - 10 + 'A' : b + '0');
    b = data[i] & 0x0f;
    *buf++ = (b > 9 ? b - 10 + 'A' : b + '0');
  }
  *buf = 0;
}
