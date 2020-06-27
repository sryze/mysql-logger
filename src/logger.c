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

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include "defs.h"
#include "error.h"
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
#define MAX_WS_MESSAGE_LEN 4096
#define MAX_WS_MESSAGES 1024
#define MAX_MESSAGE_QUEUE_SIZE 10240

#ifdef DEBUG
  #define LOGGER_PORT 23306
#else
  #define LOGGER_PORT 13306
#endif

struct ws_client {
  mutex_t mutex;
  bool connected;
  socket_t socket;
  struct sockaddr_in address;
};

struct http_resource {
  const char *path;
  const char *content_type;
  const char *data;
  size_t size;
};

struct ws_message {
  struct strbuf message;
  struct ws_message *next;
};

static bool loaded;
static FILE *log_file;
static mutex_t log_mutex;

/* HTTP -> plugin */
static volatile bool http_server_active;
socket_t http_server_socket = INVALID_SOCKET;
static thread_t http_server_thread;
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
static volatile bool ws_server_active;
static socket_t ws_server_socket = INVALID_SOCKET;
static thread_t ws_server_thread;
static struct ws_client ws_clients[MAX_WS_CLIENTS];
static mutex_t ws_clients_mutex;

/* plugin -> WebSocket */
static volatile bool messaging_active;
static thread_t message_thread;
static struct ws_message *message_queue;
static struct ws_message *message_queue_tail;
static volatile long pending_message_count;
static mutex_t message_queue_mutex;

#if MYSQL_AUDIT_INTERFACE_VERSION < 0x0302
  static volatile long query_id_counter = 1;
#endif

void sql_print_error(const char *format, ...);

static void log_printf(const char *format, ...)
{
  va_list args;

  mutex_lock(&log_mutex);

  va_start(args, format);
  vfprintf(log_file, format, args);
  va_end(args);
  fflush(log_file);

  mutex_unlock(&log_mutex);
}

static void start_server(unsigned short port,
                         socket_t *sock,
                         volatile bool *flag,
                         void (*handler)(socket_t, struct sockaddr_in *))
{
  socket_t listen_sock;
  socket_t client_sock;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int opt;
  fd_set listen_set, working_set;

  listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_sock < 0) {
    log_printf("ERROR: Could not open socket: %s\n", xstrerror(xerrno));
    return;
  }

  /*
   * Allow reuse of this socket address (port) without waiting.
   */
  opt = 1;
  setsockopt(
      listen_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);
  if (bind(listen_sock,
           (struct sockaddr *)&server_addr,
           sizeof(server_addr)) != 0) {
    close_socket(listen_sock);
    log_printf(
        "ERROR: Could not bind to port %u: %s\n",
        port,
        xstrerror(xerrno));
    return;
  }

  if (listen(listen_sock, 32) != 0) {
    close_socket(listen_sock);
    log_printf(
        "ERROR: Could not start listening for connections: %s\n",
        xstrerror(xerrno));
    return;
  }

  assert(sock != NULL);
  assert(flag != NULL);
  *sock = listen_sock;

  FD_ZERO(&listen_set);
  FD_SET(listen_sock, &listen_set);

  while (*flag) {
    int result;
    struct timeval timeout;

    memcpy(&working_set, &listen_set, sizeof(fd_set));
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; /* 10 ms */

    result = select((int)listen_sock + 1, &working_set, NULL, NULL, &timeout);
    if (result < 0) {
      log_printf("ERROR: Socket monitoring failed: %s\n", xstrerror(xerrno));
      break;
    }

    if (FD_ISSET(listen_sock, &working_set)) {
      client_sock = accept(listen_sock,
                           (struct sockaddr *)&client_addr,
                           &client_addr_len);
      if (client_sock < 0) {
        log_printf(
            "ERROR: Could not accept connection: %s\n",
            xstrerror(xerrno));
        continue;
      }
      handler(client_sock, &client_addr);
    }
  }
}

static bool perform_ws_handshake(socket_t sock)
{
  char buf[MAX_HTTP_HEADERS] = {0};
  int len;
  int error;
  const char *key;
  size_t key_len;
  char *key_copy;

  len = http_recv_headers(sock, buf, sizeof(buf));
  if (len <= 0) {
    log_printf("ERROR: Could not receive HTTP headers: %s\n",
        xstrerror(xerrno));
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
    log_printf("ERROR: %s\n",xstrerror(errno));
    http_send_internal_error(sock);
    return false;
  }

  error = ws_send_handshake_accept(sock, key_copy);
  free(key_copy);
  if (error != 0) {
    log_printf(
        "ERROR: Could not send WebSocket handshake accept response: %s\n",
        xstrerror(error));
    return false;
  }

  return true;
}

static void process_ws_request(socket_t sock, struct sockaddr_in *addr)
{
  struct ws_client *client = NULL;

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
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
      if (!ws_clients[i].connected) {
        client = &ws_clients[i];
        client->connected = true;
        client->socket = sock;
        client->address = *addr;
        mutex_create(&client->mutex);
        break;
      }
    }
  }
  mutex_unlock(&ws_clients_mutex);

  if (client == NULL) {
    log_printf("Client limit reached, closing connection\n");
    ws_send_close(sock, 0, 0);
    close_socket(sock);
  }
}

static void process_ws_requests(void *arg)
{
  UNUSED(arg);

  log_printf("Listening for WebSocket connections on port %d\n",
      LOGGER_PORT + 1);
  start_server(
      LOGGER_PORT + 1,
      &ws_server_socket,
      &ws_server_active,
      process_ws_request);
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
    log_printf("ERROR: Could not receive HTTP headers: %s\n",
        xstrerror(xerrno));
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

  log_printf("Listening for HTTP connections on port %d\n", LOGGER_PORT);
  start_server(
      LOGGER_PORT,
      &http_server_socket,
      &http_server_active,
      process_http_request);
}

static void send_event_message(const struct mysql_event_general *event_general)
{
  struct strbuf buf;
  bool ignore = false;

  mutex_lock(&message_queue_mutex);
  {
    if (pending_message_count >= MAX_MESSAGE_QUEUE_SIZE) {
      ignore = true;
    }
  }
  mutex_unlock(&message_queue_mutex);

  if (ignore) {
    return;
  }

  if (strbuf_alloc(&buf, MAX_WS_MESSAGE_LEN) != 0) {
    return;
  }

  strbuf_append(&buf, "{");

  switch (event_general->event_subclass) {
    case MYSQL_AUDIT_GENERAL_LOG:
      json_encode(&buf,
        "\"type\": %s,", "query_start");
      json_encode(&buf,
        "\"user\": %s,", event_general->general_user);
      json_encode(&buf,
        "\"query\": %s,", event_general->general_query);
      json_encode(&buf,
        "\"time\": %L,", time_ms());
      json_encode(&buf,
        "\"rows\": %L,", event_general->general_rows);
#if MYSQL_AUDIT_INTERFACE_VERSION >= 0x0302
      json_encode(&buf,
        "\"query_id\": %L,", event_general->query_id);
      json_encode(&buf,
        "\"database\": %s", *(const char * const *)&event_general->database);
#else
      json_encode(&message,
        "\"query_id\": %l", ATOMIC_INCREMENT(&query_id_counter));
#endif
      break;
    case MYSQL_AUDIT_GENERAL_ERROR:
      json_encode(&buf,
        "\"type\": %s,", "query_error");
      json_encode(&buf,
        "\"query_id\": %L,", event_general->query_id);
      json_encode(&buf,
        "\"time\": %L,", time_ms());
      json_encode(&buf,
        "\"error_code\": %i,", event_general->general_error_code);
      json_encode(&buf,
        "\"error_message\": %s", event_general->general_command);
      break;
    case MYSQL_AUDIT_GENERAL_RESULT:
      json_encode(&buf,
        "\"type\": %s,", "query_result");
      json_encode(&buf,
        "\"query_id\": %L,", event_general->query_id);
      json_encode(&buf,
        "\"time\": %L,", time_ms());
      json_encode(&buf,
        "\"rows\": %L", event_general->general_rows);
      break;
  }

  strbuf_append(&buf, "}");

  mutex_lock(&message_queue_mutex);
  {
    struct ws_message *message = malloc(sizeof(*message));
    message->message = buf;
    message->next = NULL;
    if (message_queue_tail != NULL) {
      message_queue_tail->next = message;
    }
    message_queue_tail = message;
    if (message_queue == NULL) {
      message_queue = message;
    }
  }
  mutex_unlock(&message_queue_mutex);

  ATOMIC_INCREMENT(&pending_message_count);
}

static void process_message(struct ws_message *message)
{
  for (int i = 0; i < MAX_WS_CLIENTS; i++) {
    struct ws_client *client = &ws_clients[i];

    mutex_lock(&client->mutex);
    {
      if (client->connected) {
        ws_send_text(client->socket, message->message.str, WS_FLAG_FINAL, 0);
      }
    }
    mutex_unlock(&client->mutex);
  }

  ATOMIC_DECREMENT(&pending_message_count);
}

static void process_pending_messages(void *arg)
{
  struct ws_message *message;

  UNUSED(arg);

  while (messaging_active) {
    thread_sleep(10); /* sleep for 10 ms */

    if (message_queue == NULL) {
      continue;
    }

    mutex_lock(&message_queue_mutex);
    {
      message = message_queue;
      message_queue = message->next;
      message->next = NULL;
      if (message_queue_tail == message) {
        message_queue_tail = NULL; /* last message */
      }
    }
    mutex_unlock(&message_queue_mutex);

    process_message(message);
    free(message);
  }
}

static int logger_plugin_init(void *arg)
{
  int error;

  UNUSED(arg);

  log_file = fopen("logger.log", "w");
  if (log_file == NULL) {
    fprintf(stderr, "Failed to open log file: %s\n", xstrerror(errno));
    return 1;
  }

  if (mutex_create(&log_mutex) != 0) {
    fprintf(stderr, "Failed to create log mutex: %s\n", xstrerror(xerrno));
    return 1;
  }

  if (loaded) {
    log_printf("Logger plugin is already loaded!\n");
    return 1;
  }

  log_printf("Logger plugin is initializing...\n");

  mutex_create(&ws_clients_mutex);
  mutex_create(&message_queue_mutex);

  http_server_active = true;
  error = thread_create(
      &http_server_thread, process_http_requests, NULL);
  if (error != 0) {
    log_printf("Failed to create HTTP server thread: %s\n", xstrerror(error));
    return error;
  }
  thread_set_name(ws_server_thread, "logger_http_server_thread");

  ws_server_active = true;
  error = thread_create(
      &ws_server_thread, process_ws_requests, NULL);
  if (error != 0) {
    log_printf("Failed to create WebSocket server thread: %s\n",
        xstrerror(error));
    return error;
  }
  thread_set_name(ws_server_thread, "logger_ws_server_thread");

  messaging_active = true;
  error = thread_create(
      &message_thread, process_pending_messages, NULL);
  if (error != 0) {
    log_printf("Failed to create messaging thread: %s\n", xstrerror(error));
    return error;
  }
  thread_set_name(message_thread, "logger_message_thread");

  log_printf("Logger plugin started successfully\n");
  loaded = true;

  return 0;
}

static int logger_plugin_deinit(void *arg)
{
  UNUSED(arg);

  log_printf("Logger plugin is being deinitialized...\n");
  loaded = false;

  http_server_active = false;
  thread_join(http_server_thread);
  if (http_server_socket != INVALID_SOCKET) {
    close_socket_nicely(http_server_socket);
  }

  ws_server_active = false;
  thread_join(ws_server_thread);
  if (ws_server_socket != INVALID_SOCKET) {
    close_socket_nicely(ws_server_socket);
  }

  mutex_lock(&message_queue_mutex);
  {
    while (message_queue != NULL) {
      struct ws_message *message = message_queue;
      message_queue = message->next;
      free(message);
    }
    message_queue_tail = NULL;
  }
  mutex_unlock(&message_queue_mutex);

  messaging_active = false;
  thread_join(message_thread);

  mutex_lock(&ws_clients_mutex);
  {
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
      struct ws_client *client = &ws_clients[i];

      mutex_lock(&client->mutex);
      mutex_t mutex = client->mutex;
      {
        if (client->connected) {
          close_socket_nicely(client->socket);
          client->socket = INVALID_SOCKET;
          client->connected = false;
          mutex_destroy(&client->mutex);
        }
      }
      mutex_unlock(&mutex);
      mutex_destroy(&mutex);
    }
  }
  mutex_unlock(&ws_clients_mutex);

  mutex_destroy(&ws_clients_mutex);
  mutex_destroy(&log_mutex);
  mutex_destroy(&message_queue_mutex);

  fclose(log_file);

  return 0;
}

static void logger_notify(MYSQL_THD thd,
                          unsigned int event_class,
                          const void *event)
{
  if (!loaded) {
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
        send_event_message(event_general);
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
