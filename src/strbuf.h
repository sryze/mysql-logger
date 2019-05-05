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

#ifndef STRBUF_H
#define STRBUF_H

#include <stddef.h>

struct strbuf {
  char *str;
  size_t length;
  size_t max_count;
};

int strbuf_alloc(struct strbuf *sb, size_t size);
int strbuf_alloc_default(struct strbuf *sb);
void strbuf_free(struct strbuf *sb);
int strbuf_append(struct strbuf *sb, const char *str);
int strbuf_appendn(struct strbuf *sb, const char *str, size_t len);
int strbuf_insert(struct strbuf *sb, size_t pos, const char *str);
int strbuf_insertn(struct strbuf *sb, size_t pos, const char *str, size_t len);
int strbuf_delete(struct strbuf *sb, size_t start, size_t count);

#endif /* STRBUF_H */
