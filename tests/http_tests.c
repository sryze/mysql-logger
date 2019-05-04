#include "http.h"
#include "string_ext.h"
#include "test.h"

void test_http_request_line_parsing(void)
{
  static const char request_line[] = "GET /path HTTP/1.1\r\n";
  struct http_fragment method;
  struct http_fragment target;
  int version;
  const char *result;

  result = http_parse_request_line(request_line,
                                   &method,
                                   &target,
                                   &version);
  TEST(result == request_line + sizeof(request_line) - 1);
  TEST(strncmp(method.ptr, "GET", method.length) == 0);
  TEST(strncmp(target.ptr, "/path", target.length) == 0);
  TEST(version == 0x0101);

  version = 0;
  result = http_parse_request_line(
    "GET / HTTP/2.0\r\n", NULL, NULL, &version);
  TEST(version == 0x0200);

  version = 0;
  result = http_parse_request_line(
    "GET / HTTP/1.\r\n", NULL, NULL, &version);
  TEST(version == 0x0100);

  version = 0;
  result = http_parse_request_line(
    "GET / HTTP/1\r\n", NULL, NULL, &version);
  TEST(version == 0x0100);

  version = 0;
  result = http_parse_request_line(
    "GET / HTTP/\r\n", NULL, NULL, &version);
  TEST(result == NULL);

  result = http_parse_request_line(NULL, NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line("", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line(" ", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line("  ", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line("   ", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line(" \r\n", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line(
    " /path HTTP/1.1\r\n", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line(
    "INVALID_METHOD /path HTTP/1.1\r\n", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line(
    "GET  HTTP/1.1\r\n", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line(
    "GET /path\r\n", NULL, NULL, NULL);
  TEST(result == NULL);

  result = http_parse_request_line(
    "GET /path HPPT/1.1\r\n", NULL, NULL, NULL);
  TEST(result == NULL);
}

struct data1 {
  bool have_header1;
  bool have_header2;
};

struct data3 {
  bool have_header1;
};

static void header_callback1(const struct http_fragment *name,
                             const struct http_fragment *value,
                             void *data)
{
  struct data1 *d = data;

  if (strncmp(name->ptr, "Header1", name->length) == 0
    && strncmp(value->ptr, "Value1", value->length) == 0) {
    d->have_header1 = true;
  } else if (strncmp(name->ptr, "Header2", name->length) == 0
    && strncmp(value->ptr, "Value2", value->length) == 0) {
    d->have_header2 = true;
  }
}

static void header_callback3(const struct http_fragment *name,
                             const struct http_fragment *value,
                             void *data)
{
  struct data3 *d = data;

  if (strncmp(name->ptr, "Header1", name->length) == 0
    && strncmp(value->ptr, "Value1; Value2", value->length) == 0) {
    d->have_header1 = true;
  }
}

void test_http_header_parsing(void)
{
  const char *result;
  struct data1 d1 = {false, false};
  struct data1 d2 = {false, false};
  struct data3 d3 = {false};
  static const char headers1[] = "Header1: Value1\r\nHeader2: Value2\r\n";
  static const char headers2[] =
    "Header1: Value1\r\nHeader2: Value2\r\n\r\nBody";
  static const char headers3[] =
    "Header1: Value1; Value2   \r\n\r\n";

  result = http_parse_headers(headers1, header_callback1, &d1);
  TEST(result == headers1 + sizeof(headers1) - 1);
  TEST(d1.have_header1);
  TEST(d1.have_header2);

  result = http_parse_headers(headers2, header_callback1, &d2);
  TEST(result == headers2 + sizeof(headers2) - sizeof("Body"));
  TEST(d2.have_header1);
  TEST(d2.have_header2);

  result = http_parse_headers(headers3, header_callback3, &d3);
  TEST(result == headers3 + sizeof(headers3) - 1);
  TEST(d3.have_header1);
}
