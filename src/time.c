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

#include <stdlib.h>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/time.h>
#endif

#ifdef _WIN32

/* Windows epoch starts at Jan 1, 1601; Unix epoch starts at Jan 1 1970 */
#define SEC_WINDOWS_TO_UNIX_EPCH 11644473600LL

long long time_ms(void) {
  FILETIME ft;
  LARGE_INTEGER ft_large_int;

  GetSystemTimeAsFileTime(&ft);
  ft_large_int.LowPart = ft.dwLowDateTime;
  ft_large_int.HighPart = ft.dwHighDateTime;

  return ft_large_int.QuadPart / 10000 - SEC_WINDOWS_TO_UNIX_EPCH * 1000;
}

#else /* _WIN32 */

long long time_ms(void) {
  struct timeval tv;

  if (gettimeofday(&tv, NULL) != 0) {
    return 0;
  } else {
    return (long long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
  }
}

#endif /* !_WIN32 */
