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

#ifndef THREADING_H
#define THREADING_H

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

#endif /* THREADING_H */
