/*
 * Copyright 2019 Sergey Zolotarev
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "hex.h"

void hex_encode(const char *data, size_t len, char *buf, size_t buf_size)
{
  size_t i = 0;
  size_t j = 0;

  while (j < len * 2 && j < buf_size) {
    unsigned char byte = (unsigned char)data[i];
    unsigned char nibble;

    if (j % 2 == 0) {
      nibble = (byte & 0xF0) >> 4;
    } else {
      nibble = byte & 0x0F;
      i++;
    }

    if (nibble >= 0 && nibble <= 9) {
      buf[j] = '0' + nibble;
    } else {
      buf[j] = 'a' + nibble - 10;
    }

    j++;
  }
}
