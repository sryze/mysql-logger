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

#include <stdio.h>
#include <string.h>
#include "http.h"
#include "string_ext.h"

#define count_of(a) (sizeof(a) / sizeof(a[0]))
#define CRLF "\r\n"
#define IS_SPACE(c) ((c) == ' ')
#define IS_HSPACE(c) ((c) == ' ' || (c) == '\t')
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define SKIP(x) while (p < end && (x)) p++

static const char *const http_methods[] = {
  "GET",
  "HEAD",
  "POST",
  "PUT",
  "DELETE",
  "CONNECT",
  "OPTIONS",
  "TRACE"
};

const char *http_parse_request_line(
  const char *buf,
  struct http_fragment *method,
  struct http_fragment *target,
  int *version)
{
  const char *p, *p_start;
  const char *end;
  size_t i;
  const char *v;
  int version_major;
  int version_minor;

  if (buf == NULL) {
    return NULL;
  }

  end = strstr(buf, CRLF);
  if (end == NULL) {
    return NULL;
  }

  p = buf;
  p_start = p;

  SKIP(!IS_SPACE(*p));
  if (*p != ' ' || p - p_start == 0) {
    return NULL;
  }
  if (method != NULL) {
    method->ptr = p_start;
    method->length = p - p_start;
  }

  for (i = 0; i < count_of(http_methods); i++) {
    if (strnstr(p_start,
                http_methods[i],
                p - p_start) != NULL) {
      break;
    }
  }
  if (i == count_of(http_methods)) {
    return NULL;
  }

  SKIP(IS_SPACE(*p));
  if (p == end) {
    return NULL;
  }

  p_start = p;
  SKIP(!IS_SPACE(*p));
  if (p - p_start == 0) {
    return NULL;
  }
  if (target != NULL) {
    target->ptr = p_start;
    target->length = p - p_start;
  }

  SKIP(IS_SPACE(*p));
  if (p == end || strncmp(p, "HTTP/", sizeof("HTTP/") - 1) != 0) {
    return NULL;
  }

  version_major = 0;
  p += sizeof("HTTP/") - 1;
  v = p;
  SKIP(IS_DIGIT(*p));
  if (p - v == 0) {
    return NULL;
  }
  version_major = atoin(v, p - v);

  version_minor = 0;
  if (p < end && *p == '.') {
    v = ++p;
    SKIP(IS_DIGIT(*p));
    if (p - v > 0) {
      version_minor = atoin(v, p - v);
    }
  }

  if (version != NULL) {
    *version = ((version_major & 0xFF) << 8) | (version_minor & 0xFF);
  }

  return end + sizeof(CRLF) - 1;
}

const char *http_parse_headers(
  const char *buf,
  void (*callback)(
    const struct http_fragment *name,
    const struct http_fragment *value,
    void *data),
  void *data)
{
  const char *p = buf;
  const char *end = p;
  struct http_fragment name;
  struct http_fragment value;

  while ((end = strstr(p, CRLF)) != NULL) {
    if (strncmp(p, CRLF, sizeof(CRLF) - 1) == 0) {
      /*
       * End of header fields, body begins here.
       */
      return p + sizeof(CRLF) - 1;
    }

    name.ptr = p;
    SKIP(!IS_HSPACE(*p) && *p != ':');
    name.length = p - name.ptr;
    if (name.length == 0) {
      return NULL;
    }

    SKIP(IS_HSPACE(*p));
    if (p == end || *p != ':') {
      return NULL;
    }
    p++;
    SKIP(IS_HSPACE(*p));

    value.ptr = p;
    p = end;
    while (IS_SPACE(*--p)) {}
    value.length = p - value.ptr + 1;
    if (value.length == 0) {
      return NULL;
    }

    if (callback != NULL) {
      callback(&name, &value, data);
    }

    p = end + sizeof(CRLF) - 1;
  }

  return p;
}

static int on_headers(const char *buf,
                      int len,
                      int chunk_offset,
                      int chunk_len)
{
  return strnstr(buf + chunk_offset, "\r\n\r\n", len) != NULL;
}

int http_recv_headers(socket_t sock, char *headers, size_t size)
{
  return recv_n(sock, headers, (int)size, on_headers);
}

int http_send_content(socket_t sock,
                      const char *content,
                      size_t length,
                      const char *type)
{
  char headers[128];
  int result;

  snprintf(
    headers,
    sizeof(headers),
    "HTTP/1.1 200 OK" CRLF
    "Content-Type: %s" CRLF
    "Content-Length: %zu" CRLF
    "Connection: close" CRLF
    CRLF,
    type,
    length);

  result = send_string(sock, headers);
  if (result <= 0) {
    return result;
  }

  return send_n(sock, content, (int)length);
}

int http_send_ok(socket_t sock)
{
  return send_string(sock,
                     "HTTP/1.1 200 OK" CRLF
                     "Content-Length: 0" CRLF
                     "Connection: close" CRLF
                     CRLF);
}

int http_send_bad_request_error(socket_t sock)
{
  return send_string(sock,
                     "HTTP/1.1 400 Bad Request" CRLF
                     "Content-Length: 0" CRLF
                     "Connection: close" CRLF
                     CRLF);
}

int http_send_internal_error(socket_t sock)
{
  return send_string(sock,
                     "HTTP/1.1 500 Internal Server Error" CRLF
                     "Content-Length: 0" CRLF
                     "Connection: close" CRLF
                     CRLF);
}
