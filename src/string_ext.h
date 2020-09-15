/*
 * Copyright (c) 2019-2020 Sergey Zolotarev
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

#ifndef STRING_EXT_H
#define STRING_EXT_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"

#if defined _WIN32 && (defined _MSC_VER || !defined __GNUC__)
  #if !defined _MSC_VER || _MSC_VER <= 1900
    #define snprintf _snprintf
  #endif
  #define strdup _strdup
  #define strcasecmp _stricmp
  #define strncasecmp _strnicmp
  #define strtok_r strtok_s
#endif

#if defined __APPLE__ || defined __FreeBSD__ || defined __NetBSD__ || \
    defined __OpenBSD__ || defined __DragonFly__
  #define HAVE_STRNSTR
#endif

#ifdef __linux__
  #define HAVE_STRNDUP
#endif

bool strcasebegin(const char *str, const char *substr);

#ifndef HAVE_STRNDUP
  char *strndup(const char *str, size_t len);
#endif
#ifndef HAVE_STRNSTR
  const char *strnstr(const char *str, const char *substr, size_t len);
#endif

int atoin(const char *str, size_t len);

#endif /* STRING_EXT_H */
