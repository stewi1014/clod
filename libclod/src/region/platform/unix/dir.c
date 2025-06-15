#include "../platform.h"
#include "../../error.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

enum clod_region_result dir_open(dir *d, const char *path, const struct clod_region_opts *opts) {
	int fd;
	if (opts->unix_fd)
		fd = openat(opts->unix_fd, path && path[0] ? path : ".", O_DIRECTORY | O_RDONLY);
	else
		fd = open(path && path[0] ? path : ".", O_DIRECTORY | O_RDONLY);

	if (fd < 0) {
		return region_error(CLOD_REGION_INVALID_USAGE, "Opening directory: %s", strerror(errno));
	}

	*d = (dir)fd;
	return CLOD_REGION_OK;
}
enum clod_region_result dir_rename(const dir d, const char *old_name, const char *new_name) {
	if (renameat((int)(intptr_t)d, old_name, (int)(intptr_t)d, new_name)) {
		return region_error(CLOD_REGION_INVALID_USAGE, "Renaming \"%s\" to \"%s\": %s",
			old_name, new_name, strerror(errno));
	}
	return CLOD_REGION_OK;
}
enum clod_region_result dir_close(const dir d) {
	if (close((int)(intptr_t)d)) {
		return region_error(CLOD_REGION_INVALID_USAGE, "Closing directory: %s", strerror(errno));
	}
	return CLOD_REGION_OK;
}
