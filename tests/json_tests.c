#include <string.h>
#include "json.h"
#include "test.h"

void test_json_encode_bool(void)
{
  struct strbuf json;

  strbuf_alloc_default(&json);
  json_encode_bool(&json, true);
  TEST(strcmp(json.str, "true") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_bool(&json, false);
  TEST(strcmp(json.str, "false") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_bool(&json, 123);
  TEST(strcmp(json.str, "true") == 0);
  strbuf_free(&json);
}

void test_json_encode_double(void)
{
  struct strbuf json;

  strbuf_alloc_default(&json);
  json_encode_double(&json, 1);
  TEST(strcmp(json.str, "1") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_double(&json, 0);
  TEST(strcmp(json.str, "0") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_double(&json, 1.25);
  TEST(strcmp(json.str, "1.25") == 0);
  strbuf_free(&json);
}

void test_json_encode_string(void)
{
  struct strbuf json;

  strbuf_alloc_default(&json);
  json_encode_string(&json, "hello");
  TEST(strcmp(json.str, "\"hello\"") == 0);
  json_encode_string(&json, "world");
  TEST(strcmp(json.str, "\"hello\"\"world\"") == 0);
  strbuf_free(&json);
}

void test_json_encode(void)
{
  struct strbuf json;

  strbuf_alloc_default(&json);
  json_encode(&json,
    "This is a %s. The number of days in a year is usually %f. It is %b that the Earth is flat.",
    "test", 365.0, false);
  TEST(strcmp(json.str, "This is a \"test\". The number of days in a year is usually 365. It is false that the Earth is flat.") == 0);
  strbuf_free(&json);
}

void test_json_control_char_escaping(void)
{
  struct strbuf json;

  strbuf_alloc_default(&json);
  json_encode_string(&json, "\"");
  TEST(strcmp(json.str, "\"\\\"\"") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_string(&json, "\\");
  TEST(strcmp(json.str, "\"\\\\\"") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_string(&json, "\b");
  TEST(strcmp(json.str, "\"\\b\"") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_string(&json, "\f");
  TEST(strcmp(json.str, "\"\\f\"") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_string(&json, "\n");
  TEST(strcmp(json.str, "\"\\n\"") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_string(&json, "\r");
  TEST(strcmp(json.str, "\"\\r\"") == 0);
  strbuf_free(&json);

  strbuf_alloc_default(&json);
  json_encode_string(&json, "\t");
  TEST(strcmp(json.str, "\"\\t\"") == 0);
  strbuf_free(&json);
}
