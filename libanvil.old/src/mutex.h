#pragma once
#include "config.h"

#if DISABLE_CONCURRENCY

	#define mutex void*
	#define mutex_init(m)
	#define mutex_destroy(m)
	#define mutex_lock(m)
	#define mutex_unlock(m)

#elif HAVE_PTHREAD_MUTEX

	#include <pthread.h>
	#define mutex pthread_mutex_t
	#define mutex_init(m) pthread_mutex_init(m, nullptr)
	#define mutex_destroy(m) pthread_mutex_destroy(m)
	#define mutex_lock(m) pthread_mutex_lock(m)
	#define mutex_unlock(m) pthread_mutex_unlock(m)

#elif HAVE_THREADS_MUTEX

	#include <threads.h>
	#define mutex mtx_t
	#define mutex_init(m) mtx_init(m, mtx_plain)
	#define mutex_destroy(m) mtx_destroy(m)
	#define mutex_lock(m) mtx_lock(m)
	#define mutex_unlock(m) mtx_unlock(m)

#else

	#error No mutex implementation found

#endif

