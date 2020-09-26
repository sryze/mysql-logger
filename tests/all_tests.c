#include <stdio.h>
#include "config_tests.h"
#include "http_tests.h"
#include "json_tests.h"
#include "strbuf_tests.h"
#include "string_ext_tests.h"

int main(void)
{
  test_strbuf_alloc_free();
  test_strbuf_append();
  test_strbuf_insert();
  test_strbuf_delete();

  test_json_encode_bool();
  test_json_encode_double();
  test_json_encode_string();
  test_json_encode();
  test_json_control_char_escaping();

  test_http_request_line_parsing();
  test_http_header_parsing();

  test_read_config();
  test_read_config_file();

  puts("OK");
}
