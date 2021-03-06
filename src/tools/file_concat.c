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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../strbuf.h"
#include "../string_ext.h"

static int read_file(const char *path, struct strbuf *buf)
{
  FILE *file;
  size_t size;
  size_t read_size;

  file = fopen(path, "r");
  if (file == NULL) {
    return errno;
  }

  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (strbuf_alloc(buf, size + 1) != 0) {
    fclose(file);
    return ENOMEM;
  }

  read_size = fread(buf->str, 1, size, file);
  buf->length = read_size / sizeof(char);
  buf->str[read_size] = '\0';
  return 0;
}

int main(int argc, char **argv)
{
  FILE *out_file;
  int error;
  struct strbuf buf;
  long long pos;

  if (argc < 3) {
    fprintf(stderr, "Usage: file_concat input_file output_file\n");
    return EXIT_FAILURE;
  }

  out_file = fopen(argv[2], "w");
  if (out_file == NULL) {
    fprintf(stderr, "Could not open output file: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  error = read_file(argv[1], &buf);
  if (error != 0) {
    fprintf(stderr, "Could not open input file: %s\n", strerror(error));
    return EXIT_FAILURE;
  }

  pos = 0;

  for (;;) {
    char *start, *end;
    char *path;
    struct strbuf subst_buf;

    start = strstr(buf.str + pos, "{{{{");
    if (start == NULL) {
      break;
    }

    end = strstr(start + 4, "}}}}");
    if (end == NULL) {
      break;
    }

    pos = end - buf.str;
    path = strndup(start + 4, end - start - 4);
    if (path == NULL) {
      fprintf(stderr, "Could not allocate memory\n");
      break;
    }

    error = read_file(path, &subst_buf);
    if (error != 0) {
      fprintf(stderr, "Could not read %s: %s\n", path, strerror(error));
      free(path);
      break;
    }

    free(path);

    if ((error = strbuf_delete(&buf, start - buf.str, end - start + 4)) != 0
        || (error = strbuf_insertn(&buf,
                                   start - buf.str,
                                   subst_buf.str,
                                   subst_buf.length)) != 0) {
      fprintf(stderr, "Error: %s\n", strerror(error));
      break;
    }
  }

  fwrite(buf.str, sizeof(char), buf.length, out_file);
  fclose(out_file);

  strbuf_free(&buf);

  return EXIT_SUCCESS;
}
