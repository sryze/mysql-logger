#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "defs.h"
#include "string_ext.h"
#include "test.h"

struct config_data {
  char *foo;
  char *bar;
};

static void config_callback(const char *name, const char *value, void *arg)
{
  struct config_data *data = arg;

  UNUSED(arg);

  if (strcmp(name, "foo") == 0) {
    data->foo = strdup(value);
    return;
  }

  if (strcmp(name, "bar") == 0) {
    data->bar = strdup(value);
    return;
  }
}

void test_read_config(void)
{
  struct config_data data = {NULL, NULL};
  int result;

  result = read_config("foo = 1\n\nbar = hello world   \n",
                       config_callback,
                       &data);

  TEST(result == 0);
  TEST(data.foo != NULL);
  TEST(data.bar != NULL);
  TEST(strcmp(data.foo, "1") == 0);
  TEST(strcmp(data.bar, "hello world") == 0);
}


void test_read_config_file(void)
{
  struct config_data data = {NULL, NULL};
  FILE *file;
  int result;
#ifdef _WIN32
  char *name = tmpnam(NULL);
  file = fopen(name, "wb");
#else
  char name[] = "config_test_XXXXXX";
  file = fdopen(mkstemp(name), "w");
#endif
  TEST(file != NULL);

  fputs("foo = 1\n\n", file);
  fputs("bar = hello world   \n", file);
  fclose(file);

  result = read_config_file(name, config_callback, &data);

  TEST(result == 0);
  TEST(data.foo != NULL);
  TEST(data.bar != NULL);
  TEST(strcmp(data.foo, "1") == 0);
  TEST(strcmp(data.bar, "hello world") == 0);
}
