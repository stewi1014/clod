#include <clod/region.h>
#include "region_impl.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

enum clod_region_result read_opts(struct clod_region_opts *dst, const struct clod_region_opts *src) {
	if (src->version != CLOD_REGION_VERSION) {
		return region_error(CLOD_REGION_INVALID_USAGE,
			"Invalid opts.version %d. Current version is %d. Make sure to set opts.version correctly or update the library.",
			src->version, CLOD_REGION_VERSION);
	}
	dst->version = CLOD_REGION_VERSION;

	if (src->dims) {
		if (src->dims > CLOD_REGION_DIMENSIONS_MAX) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.dims %d. Must be <= %d.", src->dims,
				CLOD_REGION_DIMENSIONS_MAX);
		}
		dst->dims = src->dims;
	} else {
		dst->dims = 2;
	}

	if (src->compression) {
		if (10 > src->compression || src->compression >= 80) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.compression %d. Valid range is 10 <= opts.compression < 80.",
				src->compression);
		}
		dst->compression = src->compression;
	} else {
		dst->compression = CLOD_REGION_COMPRESS_ZLIB;
	}

	if (src->mode) {
		if (src->mode != CLOD_REGION_MODE_RDWR && src->mode != CLOD_REGION_MODE_RDONLY) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.mode %d. Must be CLOD_REGION_MODE_RDWR or CLOD_REGION_MODE_RDONLY.",
				src->mode);
		}
		dst->mode = src->mode;
	} else {
		dst->mode = CLOD_REGION_MODE_RDWR;
	}

	if (src->unix_fd) {
		if (src->unix_fd < 0) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.unix_fd %d. Must be >= 0.",
				src->unix_fd);
		}
		dst->unix_fd = src->unix_fd;
	}

	if (src->unix_file_perms) {
		dst->unix_file_perms = src->unix_file_perms;
	}

	if (src->prefix[0]) {
		if (memchr(src->prefix, '.', CLOD_REGION_PREFIX_MAX) != nullptr) {
			return region_error(CLOD_REGION_INVALID_USAGE,
				"Invalid opts.prefix %s. Must not contain a '.' character.",
				src->prefix);
		}
		strncpy(dst->prefix, src->prefix, CLOD_REGION_PREFIX_MAX);
		dst->prefix[CLOD_REGION_PREFIX_MAX] = '\0';
	} else {
		strncpy(dst->prefix, "region", CLOD_REGION_PREFIX_MAX + 1);
	}

	if (src->region_ext[0]) {
		strncpy(dst->region_ext, src->region_ext, CLOD_REGION_EXTENSION_MAX);
		dst->region_ext[CLOD_REGION_EXTENSION_MAX] = '\0';
	} else {
		strncpy(dst->region_ext, "mcr", CLOD_REGION_EXTENSION_MAX + 1);
	}

	if (src->chunk_ext[0]) {
		strncpy(dst->chunk_ext, src->chunk_ext, CLOD_REGION_EXTENSION_MAX);
		dst->chunk_ext[CLOD_REGION_EXTENSION_MAX] = '\0';
	} else {
		strncpy(dst->chunk_ext, "mcc", CLOD_REGION_EXTENSION_MAX + 1);
	}

	return true;
}
struct clod_region *clod_region_open(const char *path, const struct clod_region_opts *opts) {
	struct clod_region *r = malloc(sizeof(struct clod_region));
	if (!r) return nullptr;

	memset(r, 0, sizeof(struct clod_region));
	if (!read_opts(&r->opts, opts)) {
		free(r);
		return nullptr;
	}

	mutex_init(&r->mtx);
	auto const res = dir_open(&r->d, path, &r->opts);
	if (res != CLOD_REGION_OK) {
		mutex_destroy(&r->mtx);
		free(r);
		return nullptr;
	}

	r->cache_len = 0;
	r->cache = nullptr;

	return r;
}
enum clod_region_result clod_region_close(struct clod_region *r) {
	if (r->inside != 0) {
		region_error(CLOD_REGION_INVALID_USAGE, "Attempted to close region while still in use.");
		exit(EXIT_FAILURE);
	}

	mutex_destroy(&r->mtx);
	auto const dir_res = dir_close(r->d);
	auto const fc_res = file_cache_destroy(r);
	free(r);
	return dir_res != CLOD_REGION_OK ? dir_res : fc_res;
}
