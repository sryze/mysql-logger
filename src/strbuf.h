/*
 * Copyright (c) 2019-2020 Sergey Zolotarev
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
