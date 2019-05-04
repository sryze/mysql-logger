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

#ifndef SOCKET_EXT_H
#define SOCKET_EXT_H

#include <stdlib.h>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef int socklen_t;
  typedef SOCKET socket_t;
  #define close_socket closesocket
#else
  #include <sys/socket.h>
  #include <netdb.h>
  #include <unistd.h>
  typedef int socket_t;
  #define close_socket close
#endif

typedef int (*recv_handler_t)(
    const char *buf, int len, int chunk_offset, int chunk_len);

int recv_n(socket_t sock, char *buf, int size, recv_handler_t handler);
int send_n(socket_t sock, const char *buf, int size);
int send_string(socket_t sock, char *s);

int last_socket_error(void);

#endif /* SOCKET_EXT_H */
