/*
 * Copyright (c) 2020 Sergey Zolotarev
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

#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "strbuf.h"

int read_config_file(const char *path, config_callback_t callback, void *arg)
{
  FILE *file;
  char c;
  long name_pos, value_pos;
  size_t name_len, value_len;
  char *name, *value;

  assert(path != NULL);
  assert(callback != NULL);

  file = fopen(path, "rb");
  if (file == NULL) {
    return errno;
  }

  do {
    name_pos = value_pos = 0;
    name_len = value_len = 0;

    /* skip whitespace to the name */
    name_pos = ftell(file);
    while (isspace((c = fgetc(file))))
      name_pos++;
    if (c == EOF) {
      break;
    }

    /* read name in */
    fseek(file, -1, SEEK_CUR);
    while (!isspace((c = fgetc(file))) && c != '=')
      name_len++;
    if (c == EOF) {
      break;
    }

    /* skip '=' and optional whitespace around it */
    if (c != '=') {
      while (isspace((c = fgetc(file))) && c != '\n' && c != '=');
      if (c == EOF) {
        break;
      }
      if (c == '\n') {
        continue;
      }
      assert(c == '=');
    }

    /* skip whitespace to the value */
    value_pos = ftell(file);
    while (isspace((c = fgetc(file))) && c != '\n');
      value_pos++;
    if (c == EOF) {
      break;
    }
    if (c == '\n') {
      continue;
    }

    /* read value in */
    fseek(file, -1, SEEK_CUR);
    while (!isspace((c = fgetc(file))) && c != '\n' && c != EOF)
      value_len++;

    name = malloc(name_len + 1);
    if (name == NULL) {
      fclose(file);
      return errno;
    }

    (void)fseek(file, name_pos, SEEK_SET);
    (void)fread(name, 1, name_len, file);
    name[name_len] = '\0';

    value = malloc(value_len + 1);
    if (value == NULL) {
      free(name);
      fclose(file);
      return errno;
    }

    (void)fseek(file, value_pos, SEEK_SET);
    (void)fread(value, 1, value_len, file);
    value[value_len] = '\0';

    callback(name, value, arg);

    free(name);
    free(value);
  }
  while (c != EOF);

  fclose(file);

  return 0;
}
