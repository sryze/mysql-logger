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
#include <stdlib.h>
#ifndef _WIN32
  #include <unistd.h>
#endif
#include "thread.h"

#define MAX_THREAD_NAME 256

struct thread_data {
  void (*start_proc)(void *data);
  void *data;
};

#ifdef _WIN32

static DWORD WINAPI thread_start(LPVOID lpParameter)
{
  struct thread_data thread_data;

  thread_data = *(struct thread_data *)lpParameter;
  free(lpParameter);
  thread_data.start_proc(thread_data.data);

  return 0;
}

#else

static void *thread_start(void *arg)
{
  struct thread_data thread_data;

  thread_data = *(struct thread_data *)arg;
  free(arg);
  thread_data.start_proc(thread_data.data);

  return NULL;
}

#endif /* _WIN32 */

int thread_create(thread_t *handle,
                  void (*start_proc)(void *),
                  void *data)
{
  struct thread_data *thread_data;

  thread_data = malloc(sizeof(*thread_data));
  if (thread_data == NULL) {
    return errno;
  }

  thread_data->start_proc = start_proc;
  thread_data->data = data;

#ifdef _WIN32
  *handle = CreateThread(NULL,
                         0,
                         thread_start,
                         thread_data,
                         0,
                         NULL);
  return *handle != NULL ? 0 : GetLastError();
#else
  *handle = 0;
  return pthread_create(handle,
                        NULL,
                        thread_start,
                        thread_data);
#endif
}

int thread_set_name(thread_t handle, const char *name)
{
#ifdef _WIN32
  WCHAR name_w[MAX_THREAD_NAME];
  HRESULT result;

  if (MultiByteToWideChar(CP_UTF8,
                          0,
                          name,
                          -1,
                          name_w,
                          sizeof(name_w)) != 0) {
    result = SetThreadDescription(handle, name_w);
    return FAILED(result) ? HRESULT_CODE(result) : 0;
  }
  return GetLastError();
#else
  return 0;
#endif
}

int thread_stop(thread_t handle)
{
#ifdef _WIN32
  if (!TerminateThread(handle, 0)) {
    return GetLastError();
  }
  return 0;
#else
  return pthread_cancel(handle);
#endif
}

int thread_join(thread_t handle)
{
#ifdef _WIN32
  DWORD result = WaitForSingleObject(handle, INFINITE);
  if (result == WAIT_FAILED) {
    return GetLastError();
  }
  return result;
#else
  return pthread_join(handle, NULL);
#endif
}

int thread_sleep(long ms)
{
#ifdef _WIN32
  Sleep(ms);
  return 0;
#else
  return usleep(ms * 1000);
#endif
}

int mutex_create(mutex_t *mutex)
{
#ifdef _WIN32
  #ifdef USE_LIGHTWEIGHT_MUTEXES
    InitializeCriticalSection(mutex);
    return 0;
  #else
    *mutex = CreateMutex(NULL, FALSE, NULL);
    return *mutex == NULL ? GetLastError() : 0;
  #endif
#else
  return pthread_mutex_init(mutex, NULL);
#endif
}

int mutex_lock(mutex_t *mutex)
{
#ifdef _WIN32
  #ifdef USE_LIGHTWEIGHT_MUTEXES
    EnterCriticalSection(mutex);
    return 0;
  #else
    return WaitForSingleObjectEx(*mutex, INFINITE, FALSE)
      == WAIT_FAILED ? GetLastError() : 0;
  #endif
#else
  return pthread_mutex_lock(mutex);
#endif
}

int mutex_unlock(mutex_t *mutex)
{
#ifdef _WIN32
  #ifdef USE_LIGHTWEIGHT_MUTEXES
    LeaveCriticalSection(mutex);
    return 0;
  #else
    return ReleaseMutex(*mutex) ? 0 : GetLastError();
  #endif
#else
  return pthread_mutex_unlock(mutex);
#endif
}

int mutex_destroy(mutex_t *mutex)
{
#ifdef _WIN32
  #ifdef USE_LIGHTWEIGHT_MUTEXES
    DeleteCriticalSection(mutex);
    #ifdef DEBUG
      ZeroMemory(mutex, sizeof(*mutex));
    #endif
    return 0;
  #else
    return CloseHandle(*mutex) ? 0 : GetLastError();
  #endif
#else
  return pthread_mutex_destroy(mutex);
#endif
}
