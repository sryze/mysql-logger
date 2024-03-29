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
#ifdef __cplusplus
  extern "C" {
#endif
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
#ifdef __cplusplus
  }
#endif

#ifdef MariaDB_PLUGIN_MATURITY_STABLE
  #define TARGET_MARIADB 1
#else
  #define TARGET_MARIADB 0
#endif

#ifndef MYSQL_PORT
  #define MYSQL_PORT 3306
#endif
#define MYSQL_LOGGER_PORT (MYSQL_PORT + 10000)
#define MAX_HTTP_HEADERS (8 * 1024) /* HTTP RFC recommends at least 8000 */
#define MAX_ACTIVE_CONNECTIONS 256
#define MAX_WS_CLIENTS 32
#define MAX_WS_MESSAGE_LEN 4096
#define MAX_WS_MESSAGES 1024
#define MAX_MESSAGE_QUEUE_SIZE 10240

#define LOG(...) log_printf("[logger] ", __VA_ARGS__)
#define LOG_ERROR(...) \
  do {\
    if (config_trace) \
      log_printf("[logger] ERROR: ", __VA_ARGS__); \
  } while (false)
#define LOG_TRACE(...) \
  do {\
    if (config_trace) \
      log_printf("[logger] TRACE: ", __VA_ARGS__); \
  } while (false)

#if TARGET_MARIADB
  #define CSTR(s) (s)
#else
  #define CSTR(s) (s).str
#endif

struct http_resource {
  const char *path;
  const char *content_type;
  const char *data;
  size_t size;
};

struct ws_client {
  mutex_t mutex;
  bool connected;
  socket_t socket;
  struct sockaddr address;
  char address_str[INET6_ADDRSTRLEN];
};

struct ws_message {
  struct strbuf message;
  struct ws_message *next;
};

static mutex_t log_mutex;
static FILE *log_file;

static int config_http_port;
static int config_ws_port;
static bool config_trace;

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

#if !TARGET_MARIADB || MYSQL_AUDIT_INTERFACE_VERSION < 0x0302
  static volatile long query_id_counter = 1;
#endif

#if TARGET_MARIADB
  typedef unsigned int mysql_event_class_t;
#endif

static void log_printf(const char *prefix, const char *format, ...)
{
  va_list args;

  mutex_lock(&log_mutex);

  printf("%s", prefix);
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  if (log_file != NULL) {
    fprintf(log_file, "%s", prefix);
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
  }

  mutex_unlock(&log_mutex);
}

static void serve(const char *tag,
                  unsigned short port,
                  socket_t *sock,
                  volatile bool *flag,
                  int (*handler)(socket_t))
{
  socket_t server_sock;
  socket_t client_sock;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int opt;
  int result;
  int i;
  pollfd_t pollfds[MAX_ACTIVE_CONNECTIONS + 1];
  pollfd_t *server_pollfd = &pollfds[MAX_ACTIVE_CONNECTIONS];

  server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sock < 0) {
    LOG_ERROR("Could not open socket: %s\n",
        xstrerror(ERROR_SYSTEM, socket_error));
    return;
  }

  /*
   * Allow reuse of this socket address (port) without waiting.
   */
  opt = 1;
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);
  if (bind(server_sock,
           (struct sockaddr *)&server_addr,
           sizeof(server_addr)) != 0) {
    close_socket(server_sock);
    LOG_ERROR("Could not bind to port %u: %s\n",
        port,
        xstrerror(ERROR_SYSTEM, socket_error));
    return;
  }

  if (listen(server_sock, 32) != 0) {
    close_socket(server_sock);
    LOG_ERROR("Could not start listening for connections: %s\n",
        xstrerror(ERROR_SYSTEM, socket_error));
    return;
  }

  assert(sock != NULL);
  assert(flag != NULL);
  *sock = server_sock;

  for (i = 0; i <= MAX_ACTIVE_CONNECTIONS; i++) {
    pollfds[i].fd = INVALID_SOCKET;
    pollfds[i].events = 0;
    pollfds[i].revents = 0;
  }

  while (*flag) {
    for (i = 0; i < MAX_ACTIVE_CONNECTIONS; i++) {
      pollfds[i].events = pollfds[i].fd != INVALID_SOCKET ? POLLIN : 0;
      pollfds[i].revents = 0;
    }

    /* Server's socket */
    server_pollfd->fd = server_sock;
    server_pollfd->events = POLLIN;
    server_pollfd->revents = 0;

    result = poll(pollfds, MAX_ACTIVE_CONNECTIONS + 1, 10);
    if (result < 0) {
      LOG_ERROR("Failed to poll socket: %s\n",
          xstrerror(ERROR_SYSTEM, socket_error));
      break;
    }

    if (result == 0) {
      continue;
    }

    if ((server_pollfd->revents & POLLIN) != 0) {
      /* Server socket is ready to accept a new client connection */
      client_sock = accept(server_sock,
                           (struct sockaddr *)&client_addr,
                           &client_addr_len);
      if (client_sock < 0) {
        LOG_ERROR("Could not accept connection: %s\n",
            xstrerror(ERROR_SYSTEM, socket_error));
      }
      for (i = 0; i < MAX_ACTIVE_CONNECTIONS; i++) {
        if (pollfds[i].fd == INVALID_SOCKET) {
          LOG_TRACE("[%s] Connection accepted: %d\n", tag, i);
          pollfds[i].fd = (int)client_sock;
          break;
        }
      }
      if (i == MAX_ACTIVE_CONNECTIONS) {
        LOG_ERROR("Reached connection count limit\n");
        continue;
      }
    }

    for (i = 0; i < MAX_ACTIVE_CONNECTIONS; i++) {
      if (pollfds[i].fd == INVALID_SOCKET) {
        continue;
      }

      if ((pollfds[i].revents & POLLHUP) != 0
            || (pollfds[i].revents & POLLERR) != 0) {
        /* Client disconnected */
        LOG_TRACE("[%s] Disconnected: %d\n", tag, i);
        close_socket(pollfds[i].fd);
        pollfds[i].fd = INVALID_SOCKET;
        continue;
      }

      if ((pollfds[i].revents & POLLIN) != 0) {
        /* One of the client sockets is ready for reading */
        if (handler(pollfds[i].fd) != 0) {
          LOG_TRACE("Disconnected: %d\n", i);
          close_socket(pollfds[i].fd);
          pollfds[i].fd = INVALID_SOCKET;
        }
        continue;
      }
    }
  }
}

static int process_http_request(socket_t sock)
{
  char buf[MAX_HTTP_HEADERS];
  int len;
  struct http_fragment http_method;
  struct http_fragment request_target;
  int http_version;
  size_t i;
  size_t resource_count = sizeof(http_resources) / sizeof(http_resources[0]);

  len = http_recv_headers(sock, buf, sizeof(buf));
  if (len < 0) {
    LOG_ERROR("Could not receive HTTP request headers: %s\n",
        xstrerror(ERROR_SYSTEM, socket_error));
    return -1;
  }
  if (len == 0) { /* EOF */
    return -1;
  }

  if (http_parse_request_line(buf,
                              &http_method,
                              &request_target,
                              &http_version) == NULL) {
    LOG_ERROR("Could not parse HTTP request\n");
    return -1;
  }

  if (http_version > 0x01FF) {
    LOG_ERROR("Unsupported HTTP version %x\n", http_version);
    http_send_bad_request_error(sock);
    return -1;
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

  return 0;
}

static void listen_http_connections(void *arg)
{
  UNUSED(arg);

  LOG("Listening for HTTP connections on port %d\n", config_http_port);
  serve(
    "http",
    config_http_port,
    &http_server_socket,
    &http_server_active,
    process_http_request);
}

static int init_ws_client(struct ws_client *client, socket_t sock)
{
  struct sockaddr addr;
  socklen_t addr_len = sizeof(addr);
  char ip_str[INET6_ADDRSTRLEN] = {0};
  int error;

  error = getpeername(sock, &addr, &addr_len);
  if (error != 0
      || inet_ntop(addr.sa_family, &addr, ip_str, sizeof(ip_str)) == NULL) {
    strncpy(ip_str, "(unknown address)", sizeof(ip_str));
    ip_str[sizeof(ip_str) - 1] = '\0';
  }

  error = mutex_create(&client->mutex);
  if (error != 0) {
    return error;
  }

  client->connected = true;
  client->socket = sock;
  client->address = addr;
  client->address_str[0] = '\0';
  strncpy(client->address_str, ip_str, sizeof(client->address_str));
  client->address_str[sizeof(client->address_str) - 1] = '\0';

  LOG("Client connected: %s\n", ip_str);

  return 0;
}

static int free_ws_client(struct ws_client *client)
{
  int error;

  error = mutex_destroy(&client->mutex);
  if (error != 0) {
    return error;
  }

  if (client->socket != INVALID_SOCKET) {
    close_socket_nicely(client->socket);
  }

  memset(client, 0, sizeof(*client));
  client->socket = INVALID_SOCKET;

  return 0;
}

static int process_ws_request(socket_t sock)
{
  int error;
  int i;
  struct ws_client *client = NULL;

  mutex_lock(&ws_clients_mutex);
  {
    for (i = 0; i < MAX_WS_CLIENTS; i++) {
      if (ws_clients[i].connected && ws_clients[i].socket == sock) {
        client = &ws_clients[i];
        break;
      }
    }
  }
  mutex_unlock(&ws_clients_mutex);

  if (client != NULL) {
    /* Incoming request from a connected WebSocket client */
    int opcode;
    error = ws_recv(sock, &opcode, NULL, NULL, NULL);
    if (error != 0) {
      LOG_ERROR("Could not receive WebSocket data from client %s: %s\n",
          client->address_str,
          xstrerror(ERROR_SYSTEM, socket_error));
      return -1;
    }
    if (opcode == WS_OP_CLOSE) {
      LOG("Client disconnected: %s\n", client->address_str);
      free_ws_client(client);
      return -1;
    }
    return 0;
  }

  error = ws_accept(sock);
  if (error != 0) {
    LOG("WebSocket handshake failed\n");
    return -1;
  }

  mutex_lock(&ws_clients_mutex);
  {
    for (i = 0; i < MAX_WS_CLIENTS; i++) {
      if (!ws_clients[i].connected) {
        client = &ws_clients[i];
        if ((error = init_ws_client(client, sock)) != 0) {
          LOG("Could not initialize client: %s\n",
              xstrerror(ERROR_SYSTEM, error));
          ws_send_close(sock, 0, 0);
          return -1;
        }
        break;
      }
    }
  }
  mutex_unlock(&ws_clients_mutex);

  if (client == NULL) {
    LOG("Client limit reached, closing connection\n");
    ws_send_close(sock, 0, 0);
    return -1;
  }

  return 0;
}

static void listen_ws_connections(void *arg)
{
  UNUSED(arg);

  LOG("Listening for WebSocket connections on port %d\n", config_ws_port);
  serve(
    "ws",
    config_ws_port,
    &ws_server_socket,
    &ws_server_active,
    process_ws_request);
}

static void send_event(const struct mysql_event_general *event_general)
{
  bool ignore = false;
  int error;
  struct strbuf json;

  mutex_lock(&message_queue_mutex);
  {
    if (pending_message_count >= MAX_MESSAGE_QUEUE_SIZE) {
      ignore = true;
    }
  }
  mutex_unlock(&message_queue_mutex);

  if (ignore) {
    LOG_TRACE("Ignoring event because of message queue overflow");
    return;
  }

  error = strbuf_alloc(&json, MAX_WS_MESSAGE_LEN);
  if (error != 0) {
    LOG_ERROR("Error allocating message JSON buffer",
      xstrerror(ERROR_SYSTEM, error));
    return;
  }

  strbuf_append(&json, "{");

  switch (event_general->event_subclass) {
    case MYSQL_AUDIT_GENERAL_LOG: {
      const char *query_str =
        CSTR(event_general->general_query) != NULL
          ? CSTR(event_general->general_query)
          : CSTR(event_general->general_command);
      json_encode(&json,
        "\"type\": %s, ", "query_start");
      json_encode(&json,
        "\"user\": %s, ", CSTR(event_general->general_user));
      json_encode(&json,
        "\"query\": %s, ", query_str);
      json_encode(&json,
        "\"time\": %L, ", time_ms());
      json_encode(&json,
        "\"rows\": %L, ", event_general->general_rows);
#if TARGET_MARIADB && MYSQL_AUDIT_INTERFACE_VERSION >= 0x0302
      json_encode(&json,
        "\"query_id\": %L, ", event_general->query_id);
      json_encode(&json,
        "\"database\": %s", *(const char * const *)&event_general->database);
#else
      json_encode(&json,
        "\"query_id\": %l", ATOMIC_INCREMENT(&query_id_counter));
#endif
      break;
    }
    case MYSQL_AUDIT_GENERAL_ERROR:
      json_encode(&json,
        "\"type\": %s, ", "query_error");
#if defined MariaDB_PLUGIN_MATURITY_STABLE && MYSQL_AUDIT_INTERFACE_VERSION >= 0x0302
      json_encode(&json,
        "\"query_id\": %L, ", event_general->query_id);
#endif
      json_encode(&json,
        "\"time\": %L, ", time_ms());
      json_encode(&json,
        "\"error_code\": %i, ", event_general->general_error_code);
      json_encode(&json,
        "\"error_message\": %s", CSTR(event_general->general_command));
      break;
    case MYSQL_AUDIT_GENERAL_RESULT:
      json_encode(&json,
        "\"type\": %s, ", "query_result");
#if TARGET_MARIADB && MYSQL_AUDIT_INTERFACE_VERSION >= 0x0302
      json_encode(&json,
        "\"query_id\": %L, ", event_general->query_id);
#endif
      json_encode(&json,
        "\"time\": %L, ", time_ms());
      json_encode(&json,
        "\"rows\": %L", event_general->general_rows);
      break;
  }

  strbuf_append(&json, "}");

  mutex_lock(&message_queue_mutex);
  {
    struct ws_message *message = (struct ws_message *)malloc(sizeof(*message));
    if (message != NULL) {
      message->message = json;
      message->next = NULL;
      if (message_queue_tail != NULL) {
        message_queue_tail->next = message;
      }
      message_queue_tail = message;
      if (message_queue == NULL) {
        message_queue = message;
      }
      pending_message_count++;
    } else {
      LOG_ERROR("Error allocating message: %s\n",
        xstrerror(ERROR_SYSTEM, errno));
    }
  }
  mutex_unlock(&message_queue_mutex);
}

static void process_pending_messages(void *arg)
{
  struct ws_message *message;
  int i;

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
      pending_message_count--;
    }
    mutex_unlock(&message_queue_mutex);

    for (i = 0; i < MAX_WS_CLIENTS; i++) {
      struct ws_client *client = &ws_clients[i];

      if (client->connected) {
        int result;

        mutex_lock(&client->mutex);
        {
          LOG_TRACE("Sending message %s to %s\n",
                    message->message.str, client->address_str);
          result = ws_send_text(client->socket,
                                message->message.str,
                                WS_FLAG_FINAL,
                                0);
          if (result <= 0) {
            LOG_ERROR("Failed to send message to %s: %s\n",
                client->address_str,
                xstrerror(ERROR_SYSTEM, socket_error));
            free_ws_client(client);
            client = NULL;
          }
        }
        if (client != NULL) {
          mutex_unlock(&client->mutex);
        }
      }
    }

    strbuf_free(&message->message);
    free(message);
  }
}

static int logger_plugin_init(void *arg)
{
  int error;

  UNUSED(arg);

  mutex_create(&log_mutex);

  log_file = fopen("logger.log", "w");
  if (log_file == NULL) {
    fprintf(stderr,
            "Could not open log file for writing: %s\n",
            xstrerror(ERROR_C, errno));
  }

  LOG("Logger plugin is initializing...\n");

  mutex_create(&ws_clients_mutex);
  mutex_create(&message_queue_mutex);

  http_server_active = true;
  error = thread_create(&http_server_thread, listen_http_connections, NULL);
  if (error != 0) {
    LOG("Failed to create HTTP server thread: %s\n",
        xstrerror(ERROR_SYSTEM, error));
    return error;
  }
  thread_set_name(ws_server_thread, "logger_http_server_thread");

  ws_server_active = true;
  error = thread_create(&ws_server_thread, listen_ws_connections, NULL);
  if (error != 0) {
    LOG("Failed to create WebSocket server thread: %s\n",
        xstrerror(ERROR_SYSTEM, error));
    return error;
  }
  thread_set_name(ws_server_thread, "logger_ws_server_thread");

  messaging_active = true;
  error = thread_create(&message_thread, process_pending_messages, NULL);
  if (error != 0) {
    LOG("Failed to create messaging thread: %s\n",
        xstrerror(ERROR_SYSTEM, error));
    return error;
  }
  thread_set_name(message_thread, "logger_message_thread");

  LOG("Logger plugin started successfully\n");

  return 0;
}

static int logger_plugin_deinit(void *arg)
{
  int i;

  UNUSED(arg);

  LOG("Logger plugin is being deinitialized...\n");

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
      strbuf_free(&message->message);
      free(message);
    }
    message_queue_tail = NULL;
  }
  mutex_unlock(&message_queue_mutex);

  messaging_active = false;
  thread_join(message_thread);

  mutex_lock(&ws_clients_mutex);
  {
    for (i = 0; i < MAX_WS_CLIENTS; i++) {
      struct ws_client *client = &ws_clients[i];
      if (client->connected) {
        free_ws_client(client);
      }
    }
  }
  mutex_unlock(&ws_clients_mutex);

  mutex_destroy(&ws_clients_mutex);
  mutex_destroy(&message_queue_mutex);

  fclose(log_file);

  return 0;
}

static
#if MYSQL_AUDIT_INTERFACE_VERSION >= 0x0400
int
#else
void
#endif
logger_notify(MYSQL_THD thd,
              mysql_event_class_t event_class,
              const void *event)
{
  if (event_class == MYSQL_AUDIT_GENERAL_CLASS) {
    const struct mysql_event_general *
        event_general = (const struct mysql_event_general *)event;
    int event_subclass = event_general->event_subclass;
    switch (event_subclass) {
      case MYSQL_AUDIT_GENERAL_LOG:
      case MYSQL_AUDIT_GENERAL_ERROR:
      case MYSQL_AUDIT_GENERAL_RESULT:
        send_event(event_general);
        break;
    }
  }
#if MYSQL_AUDIT_INTERFACE_VERSION >= 0x0400
  return 0;
#endif
}

static struct st_mysql_audit logger_descriptor = {
  MYSQL_AUDIT_INTERFACE_VERSION,
  NULL,
  logger_notify,
  {
#if TARGET_MARIADB
    MYSQL_AUDIT_GENERAL_CLASSMASK
#else
    MYSQL_AUDIT_GENERAL_ALL
#endif
  }
};

static MYSQL_SYSVAR_INT(http_port, config_http_port,
  PLUGIN_VAR_RQCMDARG, "Logger's HTTP server port (for the web UI)",
  NULL, NULL, MYSQL_LOGGER_PORT, 1, 65536, 0);

static MYSQL_SYSVAR_INT(ws_port, config_ws_port,
  PLUGIN_VAR_RQCMDARG, "Port for WebSocket connections to logger",
  NULL, NULL, MYSQL_LOGGER_PORT + 1, 1, 65536, 0);

static MYSQL_SYSVAR_BOOL(trace, config_trace,
  PLUGIN_VAR_RQCMDARG, "Enable verbose logging",
  NULL, NULL, false);

#if MYSQL_AUDIT_INTERFACE_VERSION >= 0x0400
static struct SYS_VAR *logger_sys_vars[] = {
#else
static struct st_mysql_sys_var *logger_sys_vars[] = {
#endif
  MYSQL_SYSVAR(http_port),
  MYSQL_SYSVAR(ws_port),
  MYSQL_SYSVAR(trace),
  NULL
};

#define NAME "LOGGER"
#define AUTHOR "Sergey Zolotarev"
#define DESCRIPTION "Nice query logger"

#if TARGET_MARIADB

maria_declare_plugin(logger) {
  MYSQL_AUDIT_PLUGIN,
  &logger_descriptor,
  NAME,
  AUTHOR,
  DESCRIPTION,
  PLUGIN_LICENSE_PROPRIETARY,
  logger_plugin_init,
#if MYSQL_AUDIT_INTERFACE_VERSION >= 0x0400
  NULL,
#endif
  logger_plugin_deinit,
  0x0100,
  NULL,
  logger_sys_vars,
  "1.0",
  MariaDB_PLUGIN_MATURITY_GAMMA
}
maria_declare_plugin_end;

#else /* TARGET_MARIADB */

mysql_declare_plugin(logger) {
  MYSQL_AUDIT_PLUGIN,
    &logger_descriptor,
    NAME,
    AUTHOR,
    DESCRIPTION,
    PLUGIN_LICENSE_PROPRIETARY,
    logger_plugin_init,
#if MYSQL_AUDIT_INTERFACE_VERSION >= 0x0400
    NULL,
#endif
    logger_plugin_deinit,
    0x0100,
    NULL,
    logger_sys_vars,
    NULL,
    0
}
mysql_declare_plugin_end;

#endif /* !TARGET_MARIADB */
