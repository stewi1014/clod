#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "error.h"
#include "platform.h"
#include "platform_impl.h"

int file_open(file *file, dir *dir, const char *name, bool create, region_opts *opts) {
	const mode_t perms = opts->unix_has_file_perms ? opts->unix_file_perms : S_IWUSR | S_IRUSR | S_IRGRP;

	if (opts->mode == REGION_MODE_RDONLY) {
		file->fd = openat(dir->fd, name, O_RDONLY);
	} else if (opts->mode == REGION_MODE_RDWR && create) {
		file->fd = openat(dir->fd, name, O_RDWR | O_CREAT, perms);
	} else if (opts->mode == REGION_MODE_RDWR && !create) {
		file->fd = openat(dir->fd, name, O_RDWR);
	} else {
		set_error(REGION_ERR_INVALID_USAGE, "Unsupported mode %d.", opts->mode);
		return FILE_OPEN_ERROR;
	}

	if (file->fd < 0) {
		if (!create && errno == ENOENT) return FILE_OPEN_NOT_EXIST;
		switch (errno) {
			case EDQUOT: set_error(REGION_NO_SPACE, "User's disk quota has been reached");
			case ENOSPC: set_error(REGION_NO_SPACE, "The storage device has no remaining space");
			case ENOMEM: set_error(REGION_OUT_OF_MEMORY, "No memory available");
			default: set_error(REGION_ERR_INVALID_USAGE, "%s", strerror(errno));
		}
		return FILE_OPEN_ERROR;
	}
}

void *file_get(file *file);
void *file_truncate(file *file, size_t new_size);
bool file_close(file *file);