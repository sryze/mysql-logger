/*
 * Copyright 2019 Sergey Zolotarev
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <string.h>
#include "json.h"
#include "string_ext.h"

#define JSON_MAX_DOUBLE_SIZE 25
#define JSON_MAX_LONGLONG_SIZE 40

int json_encode_bool(struct strbuf *json, bool value)
{
  return strbuf_append(json, value ? "true" : "false");
}

int json_encode_integer(struct strbuf *json, long long value)
{
  char buf[JSON_MAX_LONGLONG_SIZE];

  snprintf(buf, sizeof(buf), "%lld", value);
  return strbuf_append(json, buf);
}

int json_encode_double(struct strbuf *json, double value)
{
  char buf[JSON_MAX_DOUBLE_SIZE];

  snprintf(buf, sizeof(buf), "%g", value);
  return strbuf_append(json, buf);
}

/*
 * This function probably does not work with UTF-8 characters very well...
 */
int json_encode_string(struct strbuf *json, const char *str)
{
  size_t len;

  if (str == NULL) {
    strbuf_append(json, "null");
    return 0;
  }

  len = strlen(str);
  strbuf_append(json, "\""); /* opening quote */

  for (int i = 0; i < len; i++) {
    int error;
    if (str[i] == '"') {
      error = strbuf_append(json, "\\\"");
    } else {
      error = strbuf_appendn(json, &str[i], 1);
    }
    if (error != 0) {
      return error;
    }
  }

  strbuf_append(json, "\""); /* closing quote */

  return 0;
}

int json_encode(struct strbuf *json, const char *format, ...)
{
  va_list args;
  size_t len = strlen(format);

  va_start(args, format);
  {
    for (size_t i = 0; i < len; i++) {
      if (format[i] == '%' && i + 1 < len) {
        switch (format[++i]) {
          case 'b':
            json_encode_bool(json, va_arg(args, int));
            break;
          case 'f':
            json_encode_double(json, va_arg(args, double));
            break;
          case 'i':
            json_encode_integer(json, va_arg(args, int));
            break;
          case 'l':
            json_encode_integer(json, va_arg(args, long));
            break;
          case 'L':
            json_encode_integer(json, va_arg(args, long long));
            break;
          case 's':
            json_encode_string(json, va_arg(args, char *));
            break;
        }
      } else {
        strbuf_appendn(json, &format[i], 1);
      }
    }
  }
  va_end(args);

  return 0;
}
