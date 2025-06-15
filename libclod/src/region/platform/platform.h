#ifndef CLOD_REGION_PLATFORM_H
#define CLOD_REGION_PLATFORM_H

#include "clod_config.h"
#include <clod/region.h>
#include <stdint.h>
#include <time.h>

typedef uintptr_t dir;
typedef uintptr_t dir_iter;
typedef uintptr_t file;

enum clod_region_result dir_open(dir *d, const char *path, const struct clod_region_opts *opts);
enum clod_region_result dir_rename(dir d, const char *old_name, const char *new_name);
enum clod_region_result dir_close(dir d);

enum clod_region_result dir_iter_open(dir_iter *iter, dir d);
enum clod_region_result dir_iter_close(dir_iter iter);
enum clod_region_result dir_iter_next(dir_iter iter, const char **name);

enum clod_region_result file_open(file *f, dir d, const char *name, bool create, const struct clod_region_opts *opts);
enum clod_region_result file_get(file f, void **data, size_t *size);
enum clod_region_result file_truncate(file f, size_t new_size);
enum clod_region_result file_close(file f);

int num_procs();

#define atomic _Atomic

#if HAVE_PTHREAD
	#include <pthread.h>

	#define mutex pthread_mutex_t
	#define mutex_init(m) pthread_mutex_init(m, nullptr)
	#define mutex_destroy(m) pthread_mutex_destroy(m)
	#define mutex_lock(m) pthread_mutex_lock(m)
	#define mutex_unlock(m) pthread_mutex_unlock(m)

	#define rwmutex pthread_rwlock_t
	#define rwmutex_init(rw) pthread_rwlock_init(rw, nullptr)
	#define rwmutex_destroy(rw) pthread_rwlock_destroy(rw)
	#define rwmutex_rdlock(rw) pthread_rwlock_rdlock(rw)
	#define rwmutex_rdunlock(rw) pthread_rwlock_unlock(rw)
	#define rwmutex_wrlock(rw) pthread_rwlock_wrlock(rw)
	#define rwmutex_wrunlock(rw) pthread_rwlock_unlock(rw)
#else
	#error "Mutex implementation for this platform has not been added"
#endif

#if defined(TIME_MONOTONIC)
	#define monotonic_now(timespec) (timespec_get((timespec), TIME_MONOTONIC) == TIME_MONOTONIC)
#elif HAVE_CLOCK_GETTIME
	#define monotonic_now(timespec) (clock_gettime(CLOCK_MONOTONIC, (timespec)) == 0)
#else
	#error "Monotonic time for this platform has not been implemented"
#endif

#endif
