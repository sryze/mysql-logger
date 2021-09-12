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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "strbuf.h"

int read_config(const char *str, config_callback_t callback, void *arg)
{
  size_t i;
  size_t len;
  char c;
  size_t name_pos, value_pos;
  size_t name_len, value_len;
  char *name, *value;

  assert(str != NULL);
  assert(callback != NULL);

  len = strlen(str);

  for (i = 0; i < len; ) {
    name_pos = value_pos = 0;
    name_len = value_len = 0;

    /* skip whitespace to the name */
    name_pos = i;
    while (i < len && isspace((c = str[i++]))) {
      name_pos++;
    }
    if (i >= len || c == EOF) {
      break;
    }

    /* read name in */
    i--;
    while (i < len && !isspace((c = str[i++])) && c != '=') {
      name_len++;
    }
    if (i >= len || c == EOF) {
      break;
    }

    /* skip '=' and optional whitespace around it */
    if (c != '=') {
      while (i < len && isspace((c = str[i++])) && c != '\n' && c != '=');
      if (i >= len || c == EOF) {
        break;
      }
      if (c == '\n') {
        continue;
      }
      assert(c == '=');
    }

    /* skip whitespace to the value */
    value_pos = i;
    while (i < len && isspace((c = str[i++])) && c != '\n') {
      value_pos++;
    }
    if (i >= len || c == EOF) {
      break;
    }
    if (c == '\n') {
      continue;
    }

    /* read value in */
    i--;
    while (i < len && (c = str[i++]) != EOF && c != '\r' && c != '\n') {
      value_len++;
    }

    name = (char *)malloc(name_len + 1);
    if (name == NULL) {
      return errno;
    }

    memcpy(name, str + name_pos, name_len);
    name[name_len] = '\0';

    value = (char *)malloc(value_len + 1);
    if (value == NULL) {
      free(name);
      return errno;
    }

    memcpy(value, str + value_pos, value_len);
    value[value_len] = '\0';

    /* trim trailing whitespace */
    while (isspace(value[value_len - 1])) {
      value[--value_len] = '\0';
    }

    callback(name, value, arg);

    free(name);
    free(value);
  }

  return 0;
}

int read_config_file(const char *path, config_callback_t callback, void *arg)
{
  FILE *file;
  size_t len;
  size_t count;
  char *buf;
  int result;

  assert(path != NULL);
  assert(callback != NULL);

  file = fopen(path, "rb");
  if (file == NULL) {
    return errno;
  }

  fseek(file, 0, SEEK_END);
  len = (size_t)ftell(file);
  fseek(file, 0, SEEK_SET);

  if (len == 0) {
    fclose(file);
    return 0;
  }

  buf = (char *)malloc(len + 1);
  if (buf == NULL) {
    fclose(file);
    return errno;
  }

  count = fread(buf, 1, len, file);
  if (count < len) {
    free(buf);
    fclose(file);
    return errno;
  }

  buf[len] = '\0';
  result = read_config(buf, callback, arg);
  free(buf);
  fclose(file);

  return result;
}
