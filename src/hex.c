/*
 * Copyright (c) 2019 Sergey Zolotarev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
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
