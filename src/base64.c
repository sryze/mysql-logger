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

#include "base64.h"

#define BYTE_BITS 8
#define CHAR_BITS 6

static const char base64_padding_char = '=';

static const char base64_index_table[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9' , '+', '/'
};

size_t base64_encode(const void *data,
                     size_t len,
                     char *buf,
                     size_t buf_size)
{
  const unsigned char *bytes = data;
  size_t num_chars =
      (len * BYTE_BITS) / CHAR_BITS + ((len * BYTE_BITS) % CHAR_BITS != 0);
  size_t i;
  size_t output_len = 0;

  for (i = 0; i < num_chars && i < buf_size; i++) {
    size_t byte1_index = (i * CHAR_BITS) / BYTE_BITS;
    size_t byte2_index = ((i + 1) * CHAR_BITS) / BYTE_BITS;
    size_t bit1_index = (i * CHAR_BITS) % BYTE_BITS;
    size_t bit2_index = ((i + 1) * CHAR_BITS) % BYTE_BITS;
    unsigned char b1 = bytes[byte1_index];
    unsigned char b2 = byte2_index < len ? bytes[byte2_index] : 0;
    unsigned char index =
      ((b1 & (0xFF >> bit1_index)) << (bit1_index - 2))
      |
      (b2 >> (BYTE_BITS - bit2_index));
    buf[i] = base64_index_table[index];
  }

  output_len = (i + 4) & ~(size_t)3; /* add padding to align to 4 characters */

  for (; i < output_len && i < buf_size; i++) {
    buf[i] = base64_padding_char;
  }

  return output_len;
}
