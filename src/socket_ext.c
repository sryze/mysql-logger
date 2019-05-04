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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "socket_ext.h"

int recv_n(socket_t sock, char *buf, int size, recv_handler_t handler)
{
  int len = 0;
  int recv_len;

  for (;;) {
    if (len >= size) {
      break;
    }
    recv_len = recv(sock, buf + len, size - len, 0);
    if (recv_len <= 0) {
      return recv_len;
    }
    if (recv_len == 0) {
      break;
    }
    len += recv_len;
    if (handler != NULL && handler(buf, len, len - recv_len, recv_len)) {
      break;
    }
  }

  return len;
}

int send_n(socket_t sock, const char *buf, int size)
{
  int len = 0;
  int send_len;

  for (;;) {
    if (len >= size) {
      break;
    }
    send_len = send(sock, buf + len, size - len, 0);
    if (send_len <= 0) {
      return send_len;
    }
    if (send_len == 0) {
      break;
    }
    len += send_len;
  }

  return len;
}

int send_string(socket_t sock, char *s)
{
  size_t len = strlen(s);

  if ((size_t)len > INT_MAX) {
    return -1;
  }
  return send_n(sock, s, (int)len);
}

int last_socket_error(void)
{
#ifdef _WIN32
  return GetLastError();
#else
  return errno;
#endif
}
