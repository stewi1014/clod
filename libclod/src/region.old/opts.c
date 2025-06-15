#include "error.h"
#include <../../include/clod/region.h>
#include <alloca.h>
#include <stdlib.h>
#include <string.h>

typedef struct region_opts region_opts_v1;

const struct region_opts default_opts = {
	.version = REGION_VERSION,
	.dims = 2,
	.compression = REGION_COMPRESS_ZLIB,
	.mode = REGION_MODE_RDWR,
	.unix_has_fd = false,
	.unix_has_file_perms = false,
	.prefix = "region",
	.region_ext = "mcr",
	.chunk_ext = "mcc",
	.max_size = 107374182400 , // 100GB
	.logger_user = nullptr,
	.logger = default_logger,
	.malloc = malloc,
	.free = free,
};

bool parse_opts(struct region_opts *opts, const void *src, struct error_details *err) {
	const uint8_t version = *(const uint8_t*)src;
	switch (version) {
		case 1: {
			const region_opts_v1 *v1 = src;
			opts->version = version;
			if (v1->dims) opts->dims = v1->dims;
			if (v1->compression) opts->compression = v1->compression;
			if (v1->mode) opts->mode = v1->mode;
			if (v1->unix_has_fd) {
				opts->unix_has_fd = true;
				opts->unix_fd = v1->unix_fd;
			}
			if (v1->unix_has_file_perms) {
				opts->unix_has_file_perms = true;
				opts->unix_file_perms = v1->unix_file_perms;
			}
			if (v1->max_size) opts->max_size = v1->max_size;
			if (v1->logger_user) opts->logger_user = v1->logger_user;
			if (v1->logger) opts->logger = v1->logger;
			if (v1->malloc) opts->malloc = v1->malloc;
			if (v1->free) opts->free = v1->free;
			if (v1->prefix[0] != 0) {
				memcpy(opts->prefix, v1->prefix, REGION_PREFIX_MAX);
				opts->prefix[REGION_PREFIX_MAX] = '\0';
			};
			if (v1->region_ext[0] != 0) {
				memcpy(opts->region_ext, v1->region_ext, REGION_EXTENSION_MAX);
				opts->prefix[REGION_EXTENSION_MAX] = '\0';
			};
			if (v1->chunk_ext[0] != 0) {
				memcpy(opts->chunk_ext, v1->chunk_ext, REGION_EXTENSION_MAX);
				opts->chunk_ext[REGION_EXTENSION_MAX] = '\0';
			};
			break;
		}
		default: {
			set_error(REGION_INVALID_USAGE, "Invalid region_opts version");
			return false;
		}
	}

#define validate(expr) if (!(expr)) { set_error(REGION_INVALID_USAGE, "Validating region_opts failed: "#expr); return false; }
	validate(0 < opts->dims && opts->dims <= 10);
	validate(10 <= opts->compression && opts->compression < 80);
	validate(opts->mode == REGION_MODE_RDWR || opts->mode == REGION_MODE_RDONLY);
	validate(!opts->unix_has_fd || opts->unix_fd > 0);
	validate(opts->max_size > 0);
	validate(opts->logger != nullptr);
	validate(opts->malloc != nullptr);
	validate(opts->free != nullptr);
	validate(strchr(opts->prefix, '.') == nullptr);
	validate(strchr(opts->prefix, '/') == nullptr);
	validate(strchr(opts->region_ext, '.') == nullptr);
	validate(strchr(opts->region_ext, '/') == nullptr);
	validate(strchr(opts->chunk_ext, '.') == nullptr);
	validate(strchr(opts->chunk_ext, '/') == nullptr);
#undef validate

	return true;
}
