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
