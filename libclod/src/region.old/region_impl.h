#ifndef REGION_IMPL_H
#define REGION_IMPL_H

#include "sync.h"
#include "platform.h.old"
#include "platform_impl.h"

#include <clod/region.h>
#include <stdatomic.h>

static_assert(REGION_FILENAME_MAX == 255);

extern const struct region_opts default_opts;
bool parse_opts(struct region_opts *opts, const void *src, struct error_details *err);

char *make_filename(char *dst, const char *prefix, const char *extension, uint8_t coord_count, const int64_t *pos, char tail);
void parse_filename(const char *filename, char *prefix, char *extension, uint8_t *coord_count, int64_t *pos, char *tail);

struct region {
	struct region_opts opts;
	struct dir dir;

	mutex mtx;
	uint32_t files_len;
	uint32_t files_cap;


	struct region_file **files;
};

struct region_file {
	volatile atomic unsigned refs; // 0 = file closed, >0 = file open. refs - 1 = actual refs;

	struct file file;
	int64_t pos[10];
};
#define REGION_FILE_SIZE(dims) (sizeof(struct region_file) + ALIGN(sizeof(((struct region_file*)nullptr)->pos[0]) * (dims), alignof(struct region_file)))

// Helps make the "constant configuration option" fact obviously different from "runtime data structures"
#define O_DIMS (r->opts.dims)
#define O_COMPRESSION (r->opts.compression)
#define O_PREFIX (r->opts.prefix)
#define O_REGION_EXT (r->opts.region_ext)
#define O_CHUNK_EXT (r->opts.chunk_ext)
#define O_OPEN_FILES (r->opts.open_files)
#define O_MAX_SIZE (r->opts.max_size)

// derived configuration constants
#define O_EXTENT (get_extent(r->opts.dims))
static uint32_t get_extent(const uint8_t dims) {
	switch (dims) {
		case 1: return 1024;
		case 2: return 32;
		case 3: return 10;
		case 4: return 5;
		case 5: return 4;
		case 6: return 3;
		case 7:
		case 8:
		case 9:
		case 10: return 2;
		default: return 0;
	}
}

#endif
