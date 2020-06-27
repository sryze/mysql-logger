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

#ifndef WS_H
#define WS_H

#include "defs.h"
#include "socket_ext.h"

#define WS_PROTOCOL_VERSION 13

enum {
  WS_ERROR_HTTP_REQUEST = 1,
  WS_ERROR_HTTP_METHOD,
  WS_ERROR_HTTP_VERSION,
  WS_ERROR_WEBSOCKET_VERSION,
  WS_ERROR_NO_UPGRADE,
  WS_ERROR_NO_KEY
};

enum {
  WS_OP_CONTINUATION = 0,
  WS_OP_TEXT = 1,
  WS_OP_BINARY = 2,
  WS_OP_CLOSE = 8,
  WS_OP_PING = 9,
  WS_OP_PONG = 10
};

enum {
  WS_FLAG_FINAL = 1u << 15,
  WS_FLAG_RSV1 = 1u << 14,
  WS_FLAG_RSV2 = 1u << 13,
  WS_FLAG_RSV3 = 1u << 12,
  WS_FLAG_MASK = 1u << 7
};

const char *ws_error_message(int error);
int ws_parse_connect_request(
  const char *buf,
  size_t len,
  const char **key,
  size_t *key_len);
int ws_send_handshake_accept(socket_t sock, const char *key);
int ws_send(
  socket_t sock,
  int opcode,
  const char *data,
  size_t len,
  uint32_t flags,
  uint32_t masking_key);
int ws_send_text(
  socket_t sock,
  const char *text,
  uint16_t flags,
  uint32_t masking_key);
int ws_send_close(socket_t sock,
  uint16_t flags,
  uint32_t masking_key);

#endif /* WS_H */
