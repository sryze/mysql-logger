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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "socket_ext.h"

int close_socket_nicely(socket_t sock)
{
  int error;

  error = shutdown(sock, SHUT_RDWR);
  if (error == 0) {
    return close_socket(sock);
  }
  return error;
}

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
