/**
 * The file cache wraps the region_file_open and region_file_close methods to
 * provide a single region_file_get method.
 */
#include "region_impl.h"
#include "region_file.h"
#include "error.h"
#include <stdint.h>
#include <stdckdint.h>
#include <stdlib.h>
#include <string.h>

#define INDEX_NIL SIZE_MAX

struct file_cache {
	struct region_file *rf;
	struct timespec last_access;
	int64_t pos[];
};

CLOD_CONST static size_t file_cache_size(const uint8_t dims) {
	const size_t unaligned = sizeof(struct file_cache) + sizeof(int64_t) * dims;
	return (unaligned + alignof(struct file_cache) - 1) &~ (alignof(struct file_cache) - 1);
}
static int64_t evictable_duration_ns(const size_t cache_size) {
	constexpr int64_t evictable_max = UINT64_C(10 * 1000 * 1000 * 1000);
	static atomic int cache_max = 0;
	if (cache_max == 0) cache_max = num_procs();
	return evictable_max - evictable_max / (int64_t)cache_max * (int64_t)cache_size;
}
static int64_t timespec_diff_ns(const struct timespec from, const struct timespec to) {
	int64_t sec_diff;
	if (ckd_sub(&sec_diff, to.tv_sec, from.tv_sec))
		return to.tv_sec > from.tv_sec ? INT64_MAX : INT64_MIN;
	if (ckd_mul(&sec_diff, sec_diff, 1000000000))
		return to.tv_sec > from.tv_sec ? INT64_MAX : INT64_MIN;
	int64_t nsec_diff;
	if (ckd_sub(&nsec_diff, to.tv_nsec, from.tv_nsec))
		return to.tv_nsec > from.tv_nsec ? INT64_MAX : INT64_MIN;
	if (ckd_add(&nsec_diff, nsec_diff, sec_diff))
		return to.tv_sec > from.tv_sec ? INT64_MAX : INT64_MIN;
	return nsec_diff;
}

#define index(i) (assert(i < r->cache_len), (struct file_cache*)((char*)r->cache + i * file_cache_size(r->opts.dims)))
#define fc_size file_cache_size(r->opts.dims)
// global mutex must be held.
enum clod_region_result region_file_get(struct clod_region *r, struct region_file **rf_ptr, const int64_t *pos, const bool create) {
	struct timespec now;
	if (!monotonic_now(&now)) {
		region_error(CLOD_REGION_INVALID_USAGE, "Failed to get current time.");
		return CLOD_REGION_INVALID_USAGE;
	}

	const int64_t evictable_duration = evictable_duration_ns(r->cache_len);

	size_t empty = INDEX_NIL;
	for (size_t i = 0; i < r->cache_len; i++) {
		auto const fc = index(i);

		if (fc->rf == nullptr) {
			if (empty == INDEX_NIL) empty = i;
		} else if (memcmp(pos, fc->pos, sizeof(fc->pos[0]) * r->opts.dims) == 0) {
			fc->last_access = now;
			*rf_ptr = fc->rf;
			return CLOD_REGION_OK;
		} else if (timespec_diff_ns(fc->last_access, now) > evictable_duration) {
			auto const res = region_file_close(fc->rf);
			fc->rf = nullptr;
			if (res != CLOD_REGION_OK) return res;
		}
	}

	struct region_file *rf;
	auto const res = region_file_open(r, &rf, pos, create);
	if (res != CLOD_REGION_OK) return res;

	if (empty == INDEX_NIL) {
		void *new = malloc((r->cache_len + 1) * fc_size);
		if (!new) {
			region_error(CLOD_REGION_INVALID_USAGE, "Failed to allocate memory for file cache.");
			region_file_close(rf);
			return CLOD_REGION_INVALID_USAGE;
		}

		memcpy(new, r->cache, r->cache_len * fc_size);
		free(r->cache);
		r->cache = new;
		empty = r->cache_len++;
	}

	auto const fc = index(empty);
	fc->rf = rf;
	fc->last_access = now;
	memcpy(fc->pos, pos, sizeof(fc->pos[0]) * r->opts.dims);
	*rf_ptr = rf;

	return CLOD_REGION_OK;
}
enum clod_region_result file_cache_destroy(struct clod_region *r) {
	enum clod_region_result res = CLOD_REGION_OK;
	for (size_t i = 0; i < r->cache_len; i++) {
		auto const fc = index(i);
		if (fc->rf) {
			auto const close_res = region_file_close(fc->rf);
			if (res == CLOD_REGION_OK) res = close_res;
		}
	}
	free(r->cache);
	r->cache = nullptr;
	r->cache_len = 0;
	return res;
}
