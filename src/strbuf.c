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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "strbuf.h"

#define STRBUF_ALLOC_SIZE 128
#define STRBUF_ALLOC_MIN_MARGIN 32
#define STRBUF_SIZE_MULTIPLIER 2

int strbuf_alloc(struct strbuf *sb, size_t count)
{
  char *str;

  str = malloc(count * sizeof(char));
  if (str == NULL) {
    sb->str = NULL;
    sb->length = 0;
    sb->max_count = 0;
    return ENOMEM;
  }

  str[0] = '\0';
  sb->str = str;
  sb->length = 0;
  sb->max_count = count;
  return 0;
}

int strbuf_alloc_default(struct strbuf *sb)
{
  return strbuf_alloc(sb, STRBUF_ALLOC_SIZE);
}

static int strbuf_realloc(struct strbuf *sb, size_t new_len)
{
  size_t count;
  char *str;

  count = sb->max_count;
  while (count < new_len + 1 + STRBUF_ALLOC_MIN_MARGIN) {
    count *= STRBUF_SIZE_MULTIPLIER;
  }

  str = realloc(sb->str, count * sizeof(char));
  if (str == NULL) {
    return ENOMEM;
  }

  sb->str = str;
  sb->max_count = count;
  return 0;
}

void strbuf_free(struct strbuf *sb)
{
  free(sb->str);
  sb->max_count = 0;
  sb->length = 0;
  sb->str = NULL;
}

int strbuf_appendn(struct strbuf *sb, const char *str, size_t len)
{
  return strbuf_insertn(sb, sb->length, str, len);
}

int strbuf_append(struct strbuf *sb, const char *str)
{
  return strbuf_insertn(sb, sb->length, str, strlen(str));
}

int strbuf_insert(struct strbuf *sb, size_t pos, const char *str)
{
  return strbuf_insertn(sb, pos, str, strlen(str));
}

int strbuf_insertn(struct strbuf *sb, size_t pos, const char *str, size_t len)
{
  size_t new_len = sb->length + len;
  int error;

  if (new_len + 1 > sb->max_count) {
    error = strbuf_realloc(sb, new_len);
    if (error != 0) {
      return error;
    }
  }

  if (pos > sb->length) {
    pos = sb->length;
  }

  memcpy(sb->str + pos + len,
         sb->str + pos,
         (sb->length - pos) * sizeof(char));
  memcpy(sb->str + pos,
         str,
         len * sizeof(char));
  sb->length = new_len;
  sb->str[new_len] = '\0';
  return 0;
}

int strbuf_delete(struct strbuf *sb, size_t start, size_t count)
{
  if (start >= sb->length || start + count > sb->length) {
    return ERANGE;
  }

  memmove(sb->str + start,
          sb->str + start + count,
          sb->length - start - count);
  sb->length = sb->length - count;
  sb->str[sb->length] = '\0';
  return 0;
}
