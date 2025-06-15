#include "region_impl.h"
#include "error.h"

#include <region_impl.h>
#include <string.h>

#define COMPRESSION_VALID_RANGE 59

region_result region_get_error(region *r) {
	return r->error;
}

region *region_open(const char *path, const region_opts *opts) {
	region_opts merged_opts = default_opts;
	if (merge_opts(&merged_opts, (void*)opts))
		return static_region_error(REGION_ERR_INVALID_USAGE);
	if (validate_opts(&merged_opts))
		return static_region_error(REGION_ERR_INVALID_USAGE);

	char *region_ext = merged_opts.malloc(strlen(merged_opts.region_ext) + 1);
	char *chunk_ext = merged_opts.malloc(strlen(merged_opts.chunk_ext) + 1);
	region *r = merged_opts.malloc(sizeof(region) + sizeof(int64_t) * merged_opts.coord_count);

	if (path == nullptr || region_ext == nullptr || chunk_ext == nullptr || r == nullptr) {
		region_error(REGION_OUT_OF_MEMORY, "Allocating region handle");
		merged_opts.free(region_ext);
		merged_opts.free(chunk_ext);
		merged_opts.free(r);
		return static_region_error(REGION_OUT_OF_MEMORY);
	}

	strcpy(region_ext, merged_opts.region_ext);
	strcpy(chunk_ext, merged_opts.chunk_ext);

	r->error = REGION_OK;
	r->opts.version = merged_opts.version;
	r->opts.mode = merged_opts.mode;
	r->opts.coord_count = merged_opts.coord_count;
	r->opts.compression = merged_opts.compression;
	r->opts.unix_has_fd = merged_opts.unix_has_fd;
	r->opts.unix_has_file_perms = merged_opts.unix_has_file_perms;
	r->opts.region_ext = region_ext;
	r->opts.chunk_ext = chunk_ext;
	r->opts.logger = merged_opts.logger;
	r->opts.user = merged_opts.user;
	r->opts.unix_file_perms = merged_opts.unix_file_perms;
	r->opts.unix_fd = merged_opts.unix_fd;
	r->opts.malloc = merged_opts.malloc;
	r->opts.calloc = merged_opts.calloc;
	r->opts.free = merged_opts.free;
	r->opts.realloc = merged_opts.realloc;
	r->file_open = false;
	for (uint64_t i = 0; i < merged_opts.coord_count; i++)
		r->file_coords[i] = 0;

	const region_result error = dir_open(r, &r->dir, path);
	if (error) {
		merged_opts.free(region_ext);
		merged_opts.free(chunk_ext);
		merged_opts.free(r);
		return static_region_error(error);
	};

	return r;
}
