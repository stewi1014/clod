#include "region_impl.h"
#include <../../include/clod/region.h>

struct region *
region_open(const char *path, const struct region_opts *opts) {
	struct error_details err;
	struct region_opts parsed_opts = default_opts;

	if (!parse_opts(&parsed_opts, opts, &err)) {
		log_error(&err, parsed_opts);
		return nullptr;
	}

	struct region *r = parsed_opts.malloc(sizeof(struct region));
	if (!r) {
		set_error(&err, REGION_INVALID_USAGE, "failed to allocate memory");
		return nullptr;
	}

	r->opts = parsed_opts;
	mutex_init(&r->mtx);
	r->files_len = 0;
	r->files_cap = 0;
	r->files = nullptr;

	if (dir_open(&r->dir, path, &r->opts, &err)) {
		r->opts.free(r);
		log_error(&err, parsed_opts);
		return nullptr;
	}

	return r;
}

void region_close(struct region *region) {

}
