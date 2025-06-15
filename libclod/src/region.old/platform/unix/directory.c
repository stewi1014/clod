#include "platform.h"
#include <clod/region.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

bool dir_open(struct dir *dir, const char *path, const struct region_opts *opts, struct error_details *err) {
	if (opts->unix_has_fd)
		dir->fd = openat(opts->unix_fd, path, O_DIRECTORY | O_RDONLY);
	else
		dir->fd = open(path, O_DIRECTORY | O_RDONLY);

	if (dir->fd < 0) {
		set_error(REGION_INVALID_USAGE, "%s", strerror(errno));
		switch (errno) {
			case EMFILE: case ENFILE: set_error(REGION_TOO_MANY_FILES, "%s", strerror(errno));
			case ENOMEM: set_error(REGION_TOO_MANY_FILES, "%s", strerror(errno));
			default: set_error(REGION_ERR_INVALID_USAGE, "%s", strerror(errno));
		}
		return false;
	}

	return true;
}

void dir_close(const struct dir *dir) {
	close(dir->fd);
}
