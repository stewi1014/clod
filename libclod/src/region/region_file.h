#ifndef CLOD_REGION_FILE_H
#define CLOD_REGION_FILE_H

#include <clod/region.h>
#include "platform/platform.h"

struct region_file {
	rwmutex mtx;
	file f;
};

// Close the region file. Only called by the file cache.
enum clod_region_result region_file_close(struct region_file *f);
// Open the region file. Only called by the file cache.
enum clod_region_result region_file_open(const struct clod_region *r, struct region_file **rf_ptr, const int64_t *pos, bool create);
// Get the region file for a given position. Should not be closed - the file cache handles file lifetime.
enum clod_region_result region_file_get(struct clod_region *r, struct region_file **rf_ptr, const int64_t *pos, bool create);

#endif
