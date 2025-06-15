#ifndef CLOD_REGION_IMPL_H
#define CLOD_REGION_IMPL_H

#include <clod/region.h>
#include "platform/platform.h"

#include <assert.h>
#include <stdint.h>
#include <time.h>

struct clod_region {
	struct clod_region_opts opts;

	atomic int32_t inside;
	mutex mtx;
	dir d;

	size_t cache_len;
	struct file_cache *cache;
};

enum clod_region_result file_cache_destroy(struct clod_region *r);

#define REGION_PUBLIC_ENTER(region) do {\
	assert((region) != nullptr);\
	const int32_t inside = ++(region)->inside;\
	assert(inside > 0);\
} while (0)

#define REGION_PUBLIC_LEAVE(region) do {\
	const int32_t inside = (region)->inside--;\
	assert(inside > 0);\
} while(0)

#endif
