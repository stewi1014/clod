#ifndef MUTEX_H
#define MUTEX_H

#include "region_config.h"

#define atomic _Atomic

#if HAVE_PTHREAD

#include <pthread.h>
#define mutex pthread_mutex_t
#define MUTEX_STATIC_INIT PTHREAD_MUTEX_INITIALIZER
#define mutex_init(mutex) pthread_mutex_init(mutex, nullptr)
#define mutex_lock(mutex) pthread_mutex_lock(mutex)
#define mutex_unlock(mutex) pthread_mutex_unlock(mutex)

#elif

#include <some_windows_header.h>
#define mutex window_mutex
#define mutex_init(mutex) windows_mutex_init(mutex)
#define mutex_lock(mutex) windows_mutex_lock(mutex)
#define mutex_unlock(mutex) windows_mutex_unlock(mutex)

#else
#error Missing mutex implementation
#endif

#endif
