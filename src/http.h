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

#ifndef HTTP_H
#define HTTP_H

#include "bool.h"
#include "socket_ext.h"

struct http_fragment {
  const char *ptr;
  size_t length;
};

const char *http_parse_request_line(
  const char *buf,
  struct http_fragment *method,
  struct http_fragment *target,
  int *version);
const char *http_parse_headers(
  const char *buf,
  void (*callback)(
    const struct http_fragment *name,
    const struct http_fragment *value,
    void *data),
  void *data);
int http_recv_headers(socket_t sock, char *headers, size_t size);
int http_send_content(
  socket_t sock,
  const char *content,
  size_t length,
  const char *type);
int http_send_ok(socket_t sock);
int http_send_bad_request_error(socket_t sock);
int http_send_internal_error(socket_t sock);

#endif /* HTTP_H */
