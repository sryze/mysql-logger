/*
 * Copyright (c) 2020 Sergey Zolotarev
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

#include <errno.h>
#include <string.h>
#include "defs.h"
#include "error.h"
#ifdef _WIN32
  #include <windows.h>
#endif

#define MAX_ERROR_LENGTH 1024

static THREAD_LOCAL char static_error_buf[MAX_ERROR_LENGTH];

const char *xstrerror(error_domain domain, int error)
{
  char *buf = static_error_buf;
  size_t size = sizeof(static_error_buf);

#ifdef _WIN32

  if (domain == ERROR_SYSTEM) {
    /* Win32 or WinSock error code */
    DWORD count;
    count = FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM
          | FORMAT_MESSAGE_IGNORE_INSERTS
          | FORMAT_MESSAGE_MAX_WIDTH_MASK,
      NULL,
      error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      buf,
      size,
      NULL);
    if (count == 0) {
      return "Unknown error";
    }
    return buf;
  } else {
    /* C/POSIX error code */
    strerror_s(buf, size, error);
    return buf;
  }

#else /* _WIN32 */

  strerror_r(error, buf, size);
  return buf;

#endif /* !_WIN32 */
}
