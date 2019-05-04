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
#include <stdio.h>
#include <stdlib.h>
#include "base64.h"
#include "bool.h"
#include "http.h"
#include "sha1.h"
#include "socket_ext.h"
#include "string_ext.h"
#include "ws.h"

#define count_of(a) (sizeof(a) / sizeof(a[0]))
#define SHA1_HASH_SIZE 20
#define PAYLOAD_LENGTH_16 126
#define PAYLOAD_LENGTH_64 127

struct ws_http_handshake_state {
  bool has_upgrade_connection;
  bool upgrade_to_websocket;
  int websocket_version;
  const char *websocket_key;
  size_t websocket_key_length;
};

static const char
  ws_key_accept_magic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static const char *ws_error_messages[] = {
  "Could not parse HTTP request",
  "Incorrect HTTP method in handshake request",
  "Unsupported version of HTTP protocol",
  "Unsupported version of WebSocket protocol",
  "Request does not contain valid conneciton upgrade headers",
  "Request does not contain 'Sec-WebSocket-Key' header"
};

const char *ws_error_message(int error) {
  if (error <= 0 || error >= count_of(ws_error_messages)) {
    return "Unknown error";
  }
  return ws_error_messages[error];
}

static void on_handshake_header(const struct http_fragment *name,
                                const struct http_fragment *value,
                                void *data)
{
  struct ws_http_handshake_state *state = data;

  if (strncasecmp(name->ptr, "Connection", name->length) == 0) {
    if (strncasecmp(value->ptr, "Upgrade", value->length) == 0) {
      state->has_upgrade_connection = true;
    } else {
      char *values = strndup(value->ptr, value->length);
      char *src = values;
      char *tok_context = NULL;
      char *value;
      while ((value = strtok_r(src, ", \t", &tok_context)) != NULL) {
        if (strcasecmp(value, "Upgrade") == 0) {
          state->has_upgrade_connection = true;
          break;
        }
        src = NULL;
      }
      free(values);
    }
  } else if (strncasecmp(name->ptr, "Upgrade", name->length) == 0) {
    state->upgrade_to_websocket =
      strncasecmp(value->ptr, "websocket", value->length) == 0;
  } else if (strncasecmp(name->ptr,
                         "Sec-WebSocket-Version",
                         name->length) == 0) {
    state->websocket_version = atoin(value->ptr, value->length);
  } else if (strncasecmp(name->ptr,
                         "Sec-WebSocket-Key",
                         name->length) == 0) {
    state->websocket_key = value->ptr;
    state->websocket_key_length = value->length;
  }
}

int ws_parse_connect_request(const char *buf,
                             size_t len,
                             const char **key,
                             size_t *key_len)
{
  struct ws_http_handshake_state parse_state = {0};
  const char *result;
  struct http_fragment http_method;
  int http_version;

  result = http_parse_request_line(buf, &http_method, NULL, &http_version);
  if (result == NULL) {
    return WS_ERROR_HTTP_REQUEST;
  }

  result = http_parse_headers(result, on_handshake_header, &parse_state);
  if (result == NULL) {
    return WS_ERROR_HTTP_REQUEST;
  }

  if (strncmp(http_method.ptr, "GET", http_method.length) != 0) {
    return WS_ERROR_HTTP_METHOD;
  } else if (http_version > 0x01FF) {
    return WS_ERROR_HTTP_VERSION;
  } else if (parse_state.websocket_version != WS_PROTOCOL_VERSION) {
    return WS_ERROR_WEBSOCKET_VERSION;
  } else if (!parse_state.has_upgrade_connection
      || !parse_state.upgrade_to_websocket) {
    return WS_ERROR_NO_UPGRADE;
  } else if (parse_state.websocket_key == NULL
      || strlen(parse_state.websocket_key) == 0) {
    return WS_ERROR_NO_KEY;
  }

  *key = parse_state.websocket_key;
  *key_len = parse_state.websocket_key_length;

  return 0;
}

static ws_uint64_t ws_hton64(ws_uint64_t x)
{
#ifdef _WIN32
  return htonll(x);
#else
  union {
    uint8_t bytes[8];
    uint64_t n;
  } v;
  v.bytes[0] = (uint8_t)((x & 0xff) << 56);
  v.bytes[1] = (uint8_t)((x & 0xff00) << 48);
  v.bytes[2] = (uint8_t)((x & 0xff0000) << 40);
  v.bytes[3] = (uint8_t)((x & 0xff000000) << 32);
  v.bytes[4] = (uint8_t)((x & 0xff00000000) << 24);
  v.bytes[5] = (uint8_t)((x & 0xff0000000000) << 16);
  v.bytes[6] = (uint8_t)((x & 0xff000000000000) << 8);
  v.bytes[7] = (uint8_t)(x & 0xff00000000000000);
  return v.n;
#endif
}

int ws_send_handshake_accept(socket_t sock, const char *key)
{
  int error = 0;
  size_t key_len;
  char *key_hash_input;
  SHA1Context sha1_context;
  char key_hash[SHA1_HASH_SIZE];
  char accept[SHA1_HASH_SIZE * 2];
  size_t accept_len;
  char response[256];

  key_len = strlen(key);
  key_hash_input = malloc(sizeof(*key_hash_input)
   * (key_len + sizeof(ws_key_accept_magic)));
  if (key_hash_input == NULL) {
    return errno;
  }

  memcpy(key_hash_input, key, key_len);
  memcpy(key_hash_input + key_len,
         ws_key_accept_magic,
         sizeof(ws_key_accept_magic));

  SHA1Reset(&sha1_context);
  SHA1Input(&sha1_context,
            (uint8_t *)key_hash_input,
            (unsigned int)strlen(key_hash_input));
  SHA1Result(&sha1_context, (uint8_t *)key_hash);
  free(key_hash_input);

  accept_len = base64_encode(key_hash,
                             sizeof(key_hash),
                             accept,
                             sizeof(accept) - 1);
  accept[accept_len] = '\0';

  snprintf(response, sizeof(response),
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Accept: %s\r\n\r\n",
    accept);
  if (send_string(sock, response) < 0) {
    return last_socket_error();
  }

  return error;
}

static ws_uint8_t *ws_alloc_frame(ws_uint16_t flags,
                                  ws_uint8_t opcode,
                                  ws_uint32_t masking_key,
                                  const char *payload,
                                  ws_uint64_t payload_len,
                                  size_t *size)
{
  ws_uint8_t payload_len_high;
  ws_uint8_t payload_ext_len_size;
  ws_uint16_t header;
  ws_uint8_t *data;
  ws_uint16_t offset = 0;
  size_t frame_size;

  (void)masking_key; /* TODO: Implement masking */

  if (payload_len < PAYLOAD_LENGTH_16) {
    payload_len_high = (ws_uint8_t)payload_len;
    payload_ext_len_size = 0;
  } else if (payload_len <= (ws_uint64_t)0xFFFF) {
    payload_len_high = PAYLOAD_LENGTH_16;
    payload_ext_len_size = (ws_uint8_t)sizeof(ws_uint16_t);
  } else {
    payload_len_high = PAYLOAD_LENGTH_64;
    payload_ext_len_size = (ws_uint8_t)sizeof(ws_uint64_t);
  }

  frame_size = sizeof(header)
    + payload_ext_len_size
    + ((flags & WS_FLAG_MASK) != 0 ? sizeof(masking_key) : 0)
    + payload_len;
  data = malloc(frame_size);
  if (data == NULL) {
    return NULL;
  }

  *size = frame_size;

  header = htons(
    (flags & 0xF080) | ((opcode & 0x0F) << 8) | (payload_len_high & 0x7F));
  memcpy(data, &header, sizeof(header));
  offset += sizeof(header);

  if (payload_ext_len_size != 0) {
    switch (payload_ext_len_size) {
      case sizeof(ws_uint16_t):
        *(ws_uint16_t *)(data + offset) = htons((ws_uint16_t)payload_len);
        break;
      case sizeof(ws_uint64_t):
        *(ws_uint64_t *)(data + offset) = ws_hton64(payload_len);
        break;
    }
    offset += payload_ext_len_size;
  }

  if ((flags & WS_FLAG_MASK) != 0) {
    *(ws_uint16_t *)(data + offset) = htons(masking_key);
    offset += sizeof(ws_uint16_t);
  }

  memcpy(data + offset, payload, payload_len);

  return data;
}

int ws_send(socket_t sock,
            int opcode,
            const char *data,
            size_t len,
            ws_uint32_t flags,
            ws_uint32_t masking_key)
{
  ws_uint8_t *frame;
  size_t frame_size;
  int error;

  frame = ws_alloc_frame(
    flags, opcode, masking_key, data, len, &frame_size);
  if (frame == NULL) {
    return -1;
  }

  error = send_n(sock, (const char *)frame, (int)frame_size);
  free(frame);

  return error;
}

int ws_send_text(socket_t sock,
                 const char *text,
                 ws_uint16_t flags,
                 ws_uint32_t masking_key)
{
  return ws_send(sock, WS_OP_TEXT, text, strlen(text), flags, masking_key);
}

int ws_send_close(socket_t sock,
                  ws_uint16_t flags,
                  ws_uint32_t masking_key)
{
  return ws_send(sock, WS_OP_CLOSE, NULL, 0, 0, 0);
}
