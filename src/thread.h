/*
 * Copyright (c) 2019 Sergey Zolotarev
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

#ifndef THREAD_H
#define THREAD_H

#ifdef _WIN32
  #include <windows.h>
  typedef HANDLE thread_t;
  typedef HANDLE mutex_t;
#else
  #include <pthread.h>
  typedef pthread_t thread_t;
  typedef pthread_mutex_t mutex_t;
#endif

#if defined _WIN32
  #define ATOMIC_INCREMENT(x) InterlockedIncrement(x)
#elif defined __GNUC__
  #define ATOMIC_INCREMENT(x) __sync_fetch_and_add(x, 1)
#endif

int thread_create(thread_t *handle,
                  void (*start_proc)(void *),
                  void *data);
int thread_set_name(thread_t handle, const char *name);
int thread_stop(thread_t handle);

int mutex_create(mutex_t *mutex);
int mutex_lock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);
int mutex_destroy(mutex_t *mutex);

#endif /* THREAD_H */
