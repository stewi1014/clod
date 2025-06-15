#include "region_impl.h"
#include "region_file.h"
#include "filename.h"
#include "error.h"
#include <stdlib.h>

enum clod_region_result region_file_open(const struct clod_region *r, struct region_file **rf_ptr, const int64_t *pos, const bool create) {
	char filename[REGION_FILENAME_MAX + 1];
	filename_make(filename, r->opts.prefix, r->opts.region_ext, pos, r->opts.dims);

	file f;
	auto const res = file_open(&f, r->d, filename, create, &r->opts);
	if (res != CLOD_REGION_OK) {
		return res;
	}

	struct region_file *rf = malloc(sizeof(*rf));
	if (!rf) {
		region_error(CLOD_REGION_INVALID_USAGE, "Failed to allocate memory for region file.");
		file_close(f);
		return CLOD_REGION_INVALID_USAGE;
	}

	rwmutex_init(&rf->mtx);
	rf->f = f;
	*rf_ptr = rf;

	return CLOD_REGION_OK;
}

enum clod_region_result region_file_close(struct region_file *f) {
	rwmutex_destroy(&f->mtx);
	auto const res = file_close(f->f);
	free(f);
	return res;
}
