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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_ext.h"

bool strcasebegin(const char *str, const char *substr)
{
  return strncasecmp(str, substr, strlen(substr)) == 0;
}

#ifndef __linux__

char *strndup(const char *str, size_t len)
{
  char *str_dup;

  str_dup = malloc(sizeof(*str_dup) * (len + 1));
  if (str_dup == NULL) {
    return NULL;
  }

  memcpy(str_dup, str, len * sizeof(char));
  str_dup[len] = '\0';

  return str_dup;
}

#endif

#ifndef __APPLE__

const char *strnstr(const char *str, const char *substr, size_t len)
{
  size_t i;
  size_t sub_len = strlen(substr);

  if (len < sub_len) {
    return NULL;
  }

  for (i = 0; i <= len - sub_len; i++) {
    if (strncmp(str + i, substr, sub_len) == 0) {
      return str + i;
    }
  }

  return NULL;
}

#endif /* __APPLE__ */

int atoin(const char *str, size_t len)
{
  size_t i;
  int value = 0;
  int m = 1;

  if (str == NULL || len == 0) {
    return 0;
  }

  for (i = len; i > 0; i--) {
    char c = str[i - 1];

    if (c < '0' || c > '9') {
      return 0;
    }

    value += m * (c - '0');
    m *= 10;
  }

  return value;
}

int strprintf(char **buf, const char *format, ...)
{
  va_list args;
  int len;

  va_start(args, format);
  va_end(args);

  len = vsnprintf(NULL, 0, format, args);

  return len;
}
