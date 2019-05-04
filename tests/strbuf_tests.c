#include <string.h>
#include "strbuf.h"
#include "test.h"

void test_strbuf_alloc_free(void) {
  struct strbuf sb;

  strbuf_alloc_default(&sb);
  TEST(sb.len == 0);
  TEST(sb.max_count > 0);
  TEST(strlen(sb.str) == 0);

  strbuf_free(&sb);
  TEST(sb.max_count == 0);
  TEST(sb.len == 0);
  TEST(sb.str == NULL);

  strbuf_alloc(&sb, 4);
  TEST(sb.max_count == 4);
  TEST(sb.len == 0);
  TEST(strlen(sb.str) == 0);

  strbuf_free(&sb);
}

void test_strbuf_append(void) {
  struct strbuf sb;

  strbuf_alloc_default(&sb);
  strbuf_append(&sb, "Hello,");
  strbuf_append(&sb, " World!");
  TEST(sb.len == strlen("Hello, World!"));
  TEST(strcmp(sb.str, "Hello, World!") == 0);
  strbuf_free(&sb);

  strbuf_alloc(&sb, 4);
  strbuf_append(&sb, "This is a test.");
  TEST(sb.len == strlen("This is a test."));
  TEST(sb.max_count == 64); /* because of min. margin it's not 16 or 32 */
  strbuf_free(&sb);
}

void test_strbuf_insert(void) {
  struct strbuf sb;

  strbuf_alloc_default(&sb);
  strbuf_append(&sb, "Hello");
  strbuf_append(&sb, "World!");
  strbuf_insert(&sb, 5, ", ");
  TEST(sb.len == strlen("Hello, World!"));
  TEST(strcmp(sb.str, "Hello, World!") == 0);
  strbuf_free(&sb);

  strbuf_alloc_default(&sb);
  strbuf_append(&sb, "this is a");
  strbuf_insert(&sb, 9, " test");
  TEST(sb.len == strlen("this is a test"));
  TEST(strcmp(sb.str, "this is a test") == 0);
  strbuf_free(&sb);
}

void test_strbuf_delete(void)
{
  struct strbuf sb;

  strbuf_alloc_default(&sb);
  strbuf_append(&sb, "this is a bad test here");
  strbuf_delete(&sb, 10, 4);
  TEST(sb.len == strlen("this is a test here"));
  TEST(strcmp(sb.str, "this is a test here") == 0);
  strbuf_free(&sb);
}
