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
  int i;
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
