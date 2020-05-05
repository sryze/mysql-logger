/*
 * Copyright (c) 2019 Sergey Zolotarev
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

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include "bool.h"
#include "http.h"
#include "json.h"
#include "socket_ext.h"
#include "strbuf.h"
#include "string_ext.h"
#include "time.h"
#include "thread.h"
#include "ui_favicon_ico.h"
#include "ui_index_html.h"
#include "ui_index_css.h"
#include "ui_index_js.h"
#include "ws.h"
#ifndef _WIN32
  #include <arpa/inet.h>
#endif

#define UNUSED(x) (void)(x)
#define MAX_HTTP_HEADERS (8 * 1024) /* HTTP RFC recommends at least 8000 */
#define MAX_WS_CLIENTS 32

#ifdef DEBUG
  #define LOGGER_PORT 23306
#else
  #define LOGGER_PORT 13306
#endif

struct ws_client {
  bool is_connected;
  socket_t socket;
  struct sockaddr_in address;
};

struct http_resource {
  const char *path;
  const char *content_type;
  const char *data;
  size_t size;
};

#if 0

struct ws_message {
  struct mysql_event_general event;
};

#endif

static bool is_plugin_ready;
static mutex_t log_mutex;

/* HTTP -> plugin */
socket_t http_server_socket = -1;
static thread_t http_server_thread;
static bool listen_http_connections;
static struct http_resource http_resources[] = {
  {
    "/",
    "text/html",
    ui_index_html,
    sizeof(ui_index_html) - 1
  },
  {
    "/favicon.ico",
    "image/x-icon",
    ui_favicon_ico,
    sizeof(ui_favicon_ico) - 1
  },
  {
    "/index.css",
    "text/css",
    ui_index_css,
    sizeof(ui_index_css) - 1
  },
  {
    "/index.js",
    "text/javascript",
    ui_index_js,
    sizeof(ui_index_js) - 1
  },
};

/* WebSocket -> plugin */
static socket_t ws_server_socket = -1;
static thread_t ws_server_thread;
static bool listen_ws_connections;
static struct ws_client ws_clients[MAX_WS_CLIENTS];
static mutex_t ws_clients_mutex;

#if 0

/* plugin -> WebSocket */
static struct ws_message *ws_message_queue;
static thread_t ws_message_thread;

#endif

#if MYSQL_AUDIT_INTERFACE_VERSION < 0x0302
  static long query_id_counter = 1;
#endif

static void log_printf(const char *format, ...)
{
  va_list args;

  mutex_lock(&log_mutex);

  printf("LOGGER: ");
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  fflush(stdout);

  mutex_unlock(&log_mutex);
}

static void start_server(unsigned short port,
                         socket_t *listen_sock,
                         bool *listen_connections,
                         void (*handler)(socket_t, struct sockaddr_in *))
{
  socket_t server_sock;
  socket_t client_sock;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int opt_reuseaddr;

  server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sock < 0) {
    log_printf("ERROR: Could not open socket: %d\n", last_socket_error());
    return;
  }

  /*
   * Allow reuse of this socket address (port) - this makes restarts faster.
   *
   * Sockets are usually waited on for some time by the system after they are
   * closed (netstat shows them in a TIME_WAIT state).
   */
  opt_reuseaddr = 1;
  setsockopt(server_sock,
             SOL_SOCKET,
             SO_REUSEADDR,
             (const void *)&opt_reuseaddr,
             sizeof(opt_reuseaddr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);
  if (bind(server_sock,
           (struct sockaddr *)&server_addr,
           sizeof(server_addr)) != 0) {
    close_socket(server_sock);
    log_printf("ERROR: Could not bind to port %u: %d\n",
               port,
               last_socket_error());
    return;
  }

  if (listen(server_sock, 1) != 0) {
    close_socket(server_sock);
    log_printf(
      "ERROR: Could not start listening for connections: %d\n",
      last_socket_error());
    return;
  }

  assert(listen_sock != NULL);
  assert(listen_connections != NULL);
  *listen_sock = server_sock;

  while (*listen_connections) {
    client_sock = accept(server_sock,
                         (struct sockaddr *)&client_addr,
                         &client_addr_len);
    if (client_sock < 0) {
      log_printf("ERROR: Could not accept connection: %d\n",
                 last_socket_error());
      continue;
    }
    handler(client_sock, &client_addr);
  }
}

static bool perform_ws_handshake(socket_t sock)
{
  int error;
  char buf[MAX_HTTP_HEADERS] = {0};
  int len;
  const char *key;
  size_t key_len;
  char *key_copy;

  len = http_recv_headers(sock, buf, sizeof(buf));
  if (len <= 0) {
    log_printf("ERROR: Could not receive HTTP headers: %d\n",
               last_socket_error());
    return false;
  }

  error = ws_parse_connect_request(buf, (size_t)len, &key, &key_len);
  if (error != 0) {
    log_printf("ERROR: WebSocket handshake error: %s\n",
               ws_error_message(error));
    http_send_bad_request_error(sock);
    return false;
  }

  key_copy = strndup(key, key_len);
  if (key_copy == NULL) {
    log_printf("ERROR: %s\n", strerror(errno));
    http_send_internal_error(sock);
    return false;
  }

  error = ws_send_handshake_accept(sock, key_copy);
  free(key_copy);
  if (error != 0) {
    log_printf(
      "ERROR: Could not send WebSocket handshake accept response: %d\n",
      error);
    return false;
  }

  return true;
}

static void process_ws_request(socket_t sock, struct sockaddr_in *addr)
{
  if (!perform_ws_handshake(sock)) {
    close_socket(sock);
    return;
  }

  char ip_str[INET_ADDRSTRLEN] = { 0 };
  if (inet_ntop(addr->sin_family,
                &addr->sin_addr,
                ip_str,
                sizeof(ip_str)) != NULL) {
    log_printf("WebSocket client connected: %s\n", ip_str);
  } else {
    log_printf("WebSocket client connected (unknown address)\n");
  }

  mutex_lock(&ws_clients_mutex);
  {
    struct ws_client *client = NULL;
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
      if (!ws_clients[i].is_connected) {
        client = &ws_clients[i];
        client->socket = sock;
        client->address = *addr;
        client->is_connected = true;
        break;
      }
    }
    if (client == NULL) {
      log_printf("Client limit reached, closing connection\n");
      ws_send_close(sock, 0, 0);
      close_socket(sock);
    }
  }
  mutex_unlock(&ws_clients_mutex);
}

static void process_http_request(socket_t sock, struct sockaddr_in *addr)
{
  char buf[MAX_HTTP_HEADERS];
  int len;
  struct http_fragment http_method;
  struct http_fragment request_target;
  int http_version;
  size_t i;
  size_t resource_count = sizeof(http_resources) / sizeof(http_resources[0]);

  len = http_recv_headers(sock, buf, sizeof(buf));
  if (len <= 0) {
    log_printf("ERROR: Could not receive HTTP headers: %d\n",
               last_socket_error());
    return;
  }

  if (http_parse_request_line(buf,
                              &http_method,
                              &request_target,
                              &http_version) == NULL) {
    log_printf("ERROR: Could not parse HTTP request\n");
    return;
  }

  if (http_version > 0x01FF) {
    log_printf("ERROR: Unsupported HTTP version %x\n", http_version);
    http_send_bad_request_error(sock);
    return;
  }

  for (i = 0; i < resource_count; i++) {
    struct http_resource *resource = &http_resources[i];

    if (strncmp(request_target.ptr,
                resource->path,
                request_target.length) == 0) {
      if (strncmp(http_method.ptr, "GET", http_method.length) == 0) {
        http_send_content(sock,
                          resource->data,
                          resource->size,
                          resource->content_type);
      } else if (strncmp(http_method.ptr, "HEAD", http_method.length) == 0) {
        http_send_ok(sock);
      } else {
        http_send_bad_request_error(sock);
      }
      break;
    }
  }

  if (i == resource_count) {
    http_send_bad_request_error(sock);
  }
}

static void process_http_requests(void *arg)
{
  UNUSED(arg);

  start_server(LOGGER_PORT, &http_server_socket, &listen_http_connections, process_http_request);
}

static void process_ws_requests(void *arg)
{
  UNUSED(arg);

  start_server(LOGGER_PORT + 1, &ws_server_socket, &listen_ws_connections, process_ws_request);
}

#if 0

static void process_ws_messages(void *arg)
{
  UNUSED(arg);
}

#endif

static void send_event(const struct mysql_event_general *event_general)
{
  struct strbuf message;

  if (strbuf_alloc(&message, 1024) != 0) {
    return;
  }

  strbuf_append(&message, "{");

  switch (event_general->event_subclass) {
    case MYSQL_AUDIT_GENERAL_LOG:
      json_encode(&message,
        "\"type\": %s,", "query_start");
      json_encode(&message,
        "\"user\": %s,", event_general->general_user);
      json_encode(&message,
        "\"query\": %s,", event_general->general_query);
      json_encode(&message,
        "\"time\": %L,", time_ms());
      json_encode(&message,
        "\"rows\": %L,", event_general->general_rows);
#if MYSQL_AUDIT_INTERFACE_VERSION >= 0x0302
      json_encode(&message,
        "\"query_id\": %L,", event_general->query_id);
      json_encode(&message,
        "\"database\": %s", *(const char * const *)&event_general->database);
#else
      json_encode(&message,
        "\"query_id\": %l", ATOMIC_INCREMENT(&query_id_counter));
#endif
      break;
    case MYSQL_AUDIT_GENERAL_ERROR:
      json_encode(&message,
        "\"type\": %s,", "query_error");
      json_encode(&message,
        "\"query_id\": %L,", event_general->query_id);
      json_encode(&message,
        "\"time\": %L,", time_ms());
      json_encode(&message,
        "\"error_code\": %i,", event_general->general_error_code);
      json_encode(&message,
        "\"error_message\": %s", event_general->general_command);
      break;
    case MYSQL_AUDIT_GENERAL_RESULT:
      json_encode(&message,
        "\"type\": %s,", "query_result");
      json_encode(&message,
        "\"query_id\": %L,", event_general->query_id);
      json_encode(&message,
        "\"time\": %L,", time_ms());
      json_encode(&message,
        "\"rows\": %L", event_general->general_rows);
      break;
  }

  strbuf_append(&message, "}");

  mutex_lock(&ws_clients_mutex);
  {
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
      struct ws_client *client = &ws_clients[i];
      if (client->is_connected) {
        ws_send_text(client->socket, message.str, WS_FLAG_FINAL, 0);
      }
    }
  }
  mutex_unlock(&ws_clients_mutex);

  strbuf_free(&message);
}

static int close_socket_nicely(socket_t sock)
{
  int error;

  error = shutdown(sock, SHUT_RDWR);
  if (error == 0) {
    return close_socket(sock);
  }
  return error;
}

static int logger_plugin_init(void *arg)
{
  UNUSED(arg);

  if (is_plugin_ready) {
    return 1;
  }

  mutex_create(&log_mutex);
  mutex_create(&ws_clients_mutex);

  int error = thread_create(&http_server_thread,
                            process_http_requests,
                            NULL);
  if (error != 0) {
    log_printf("Failed to create HTTP server thread: %d\n", error);
    return error;
  }

  thread_set_name(ws_server_thread, "logger_http_server_thread");
  listen_http_connections = true;

  error = thread_create(&ws_server_thread,
                        process_ws_requests,
                        NULL);
  if (error != 0) {
    log_printf("Failed to create WebSocket server thread: %d\n", error);
    return error;
  }

  thread_set_name(ws_server_thread, "logger_ws_server_thread");
  listen_ws_connections = true;

#if 0
  error = thread_create(&ws_message_thread,
                        process_ws_messages,
                        NULL);
  if (error != 0) {
    log_printf("Failed to create WebSocket messaging thread: %d\n", error);
    return error;
  }

  thread_set_name(ws_message_thread, "logger_ws_message_thread");
#endif

  log_printf("Logger plugin initialized successfully\n");
  is_plugin_ready = true;

  return 0;
}

static int logger_plugin_deinit(void *arg)
{
  UNUSED(arg);

  log_printf("Logger plugin is being deinitialized...\n");
  is_plugin_ready = false;

#if 0
  thread_stop(ws_message_thread);
#endif

  listen_http_connections = false;
  thread_stop(http_server_thread);
  if (http_server_socket != -1) {
    close_socket_nicely(http_server_socket);
  }

  listen_ws_connections = false;
  thread_stop(ws_server_thread);
  if (ws_server_socket != -1) {
    close_socket_nicely(ws_server_socket);
  }

  mutex_lock(&ws_clients_mutex);

  for (int i = 0; i < MAX_WS_CLIENTS; i++) {
    struct ws_client *client = &ws_clients[i];
    if (client->is_connected) {
      close_socket_nicely(client->socket);
      client->socket = -1;
      client->is_connected = false;
    }
  }

  mutex_destroy(&ws_clients_mutex);
  mutex_destroy(&log_mutex);

  return 0;
}

static void logger_notify(MYSQL_THD thd,
                          unsigned int event_class,
                          const void *event)
{
  if (!is_plugin_ready) {
    return;
  }

  if (event_class == MYSQL_AUDIT_GENERAL_CLASS) {
    const struct mysql_event_general *
        event_general = (const struct mysql_event_general *)event;
    int event_subclass = event_general->event_subclass;
    switch (event_subclass) {
      case MYSQL_AUDIT_GENERAL_LOG:
      case MYSQL_AUDIT_GENERAL_ERROR:
      case MYSQL_AUDIT_GENERAL_RESULT: {
        send_event(event_general);
        break;
      }
    }
  }
}

static struct st_mysql_audit logger_descriptor = {
  MYSQL_AUDIT_INTERFACE_VERSION,
  NULL,
  logger_notify,
  {MYSQL_AUDIT_GENERAL_CLASSMASK | MYSQL_AUDIT_TABLE_CLASSMASK}
};

mysql_declare_plugin(logger) {
  MYSQL_AUDIT_PLUGIN,         /* type                            */
  &logger_descriptor,         /* descriptor                      */
  "LOGGER",                   /* name                            */
  "Sergey Zolotarev",         /* author                          */
  "Nice query logger",        /* description                     */
  PLUGIN_LICENSE_PROPRIETARY,
  logger_plugin_init,         /* init function (when loaded)     */
  logger_plugin_deinit,       /* deinit function (when unloaded) */
  0x0001,                     /* version                         */
  NULL,                       /* status variables                */
  NULL,                       /* system variables                */
  NULL,
  MariaDB_PLUGIN_MATURITY_STABLE
}
mysql_declare_plugin_end;
