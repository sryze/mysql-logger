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
  return GetLastError();
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

int mutex_create(mutex_t *mutex)
{
#ifdef _WIN32
  *mutex = CreateMutex(NULL, FALSE, NULL);
  return *mutex == NULL ? GetLastError() : 0;
#else
  return pthread_mutex_init(mutex, NULL);
#endif
}

int mutex_lock(mutex_t *mutex)
{
#ifdef _WIN32
  return WaitForSingleObjectEx(*mutex, INFINITE, FALSE)
      == WAIT_FAILED ? GetLastError() : 0;
#else
  return pthread_mutex_lock(mutex);
#endif
}

int mutex_unlock(mutex_t *mutex)
{
#ifdef _WIN32
  return ReleaseMutex(*mutex) ? 0 : GetLastError();
#else
  return pthread_mutex_unlock(mutex);
#endif
}

int mutex_destroy(mutex_t *mutex)
{
#ifdef _WIN32
  return CloseHandle(*mutex) ? 0 : GetLastError();
#else
  return pthread_mutex_destroy(mutex);
#endif
}
