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

#ifndef STRING_EXT_H
#define STRING_EXT_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "bool.h"

#if defined _WIN32 && (defined _MSC_VER || !defined __GNUC__)
  #if !defined _MSC_VER || _MSC_VER <= 1900
    #define snprintf _snprintf
  #endif
  #define strcasecmp _stricmp
  #define strncasecmp _strnicmp
  #define strtok_r strtok_s
#endif

bool strcasebegin(const char *str, const char *substr);

#ifndef __linux__
  char *strndup(const char *str, size_t len);
#endif
#ifndef __APPLE__
  const char *strnstr(const char *str, const char *substr, size_t len);
#endif

int atoin(const char *str, size_t len);

#if 0

#define strprintf(buf_ptr, len_ptr, format, ...) \
  do { \
    size_t len; \
    len = snprintf(NULL, 0, format, __VA__ARGS__); \
  } while (0)

#endif

int strprintf(char **buf, const char *format, ...);

#endif /* STRING_EXT_H */
