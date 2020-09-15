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

#ifndef SOCKET_EXT_H
#define SOCKET_EXT_H

#include <stdlib.h>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define SHUT_RD SD_RECEIVE
  #define SHUT_WR SD_SEND
  #define SHUT_RDWR SD_BOTH
  #define close_socket closesocket
  #define ioctl_socket ioctlsocket
  #define poll WSAPoll
  typedef int socklen_t;
  typedef SOCKET socket_t;
  typedef WSAPOLLFD pollfd_t;
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/ioctl.h>
  #include <sys/select.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <poll.h>
  #define close_socket close
  #define ioctl_socket ioctl
  #define INVALID_SOCKET -1
  typedef int socket_t;
  typedef struct pollfd pollfd_t;
#endif

#define socket_error get_socket_error()
#define socket_errno get_socket_errno()

typedef int (*recv_handler_t)(
  const char *buf, int len, int chunk_offset, int chunk_len);

int get_socket_error(void);
int get_socket_errno(void);

int recv_n(
  socket_t sock, char *buf, int size, int flags, recv_handler_t handler);
int send_n(socket_t sock, const char *buf, int size, int flags);
int send_string(socket_t sock, char *s);

int close_socket_nicely(socket_t sock);

#endif /* SOCKET_EXT_H */
