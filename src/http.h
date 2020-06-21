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

#ifndef HTTP_H
#define HTTP_H

#include "socket_ext.h"
#include "types.h"

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
