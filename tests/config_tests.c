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

void test_config_parsing(void)
{
  struct config_data data = {NULL, NULL};

  char *name = tmpnam(NULL);
  FILE *file = fopen(name, "w");
  fputs("foo = 1\n\n", file);
  fputs("bar = hello world   \n", file);
  fclose(file);

  read_config_file(name, config_callback, &data);

  TEST(strcmp(data.foo, "1") == 0);
  TEST(strcmp(data.bar, "hello world") == 0);
}
