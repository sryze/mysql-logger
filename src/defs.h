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

#ifndef DEFS_H
#define DEFS_H

#include <stddef.h>

#ifdef _MSC_VER
  typedef signed __int8 int8_t;
  typedef unsigned __int8 uint8_t;
  typedef signed __int16 int16_t;
  typedef unsigned __int16 uint16_t;
  typedef signed __int32 int32_t;
  typedef unsigned __int32 uint32_t;
  typedef signed __int64 int64_t;
  typedef unsigned __int64 uint64_t;
#else
  #include <stdint.h>
#endif

#if defined __STDC__ && defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
  #include <stdbool.h>
#else
  typedef unsigned char bool;
  #define true 1
  #define false 0
  #define __bool_true_false_are_defined
#endif

#if defined _MSC_VER
  #define THREAD_LOCAL __declspec(thread)
#elif defined __GNUC__
  #define THREAD_LOCAL __thread
#endif

#endif /* DEFS_H */
