#include "../platform.h"
#include "../../error.h"
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

enum clod_region_result dir_iter_open(dir_iter *iter, const dir d) {
	auto fd = (int)(intptr_t)d;
	fd = fcntl(fd, F_DUPFD, 0);
	if (fd < 0) {
		return region_error(CLOD_REGION_INVALID_USAGE, "Opening directory iterator: %s", strerror(errno));
	}
	DIR *dirp = fdopendir(fd);
	if (!dirp) {
		region_error(CLOD_REGION_INVALID_USAGE, "Iterating over directory: %s", strerror(errno));
		close(fd);
		return CLOD_REGION_INVALID_USAGE;
	}
	*iter = (uintptr_t)dirp;
	return CLOD_REGION_OK;
}
enum clod_region_result dir_iter_close(const dir_iter iter) {
	auto const dirp = (DIR *)iter;
	if (closedir(dirp)) {
		return region_error(CLOD_REGION_INVALID_USAGE, "Closing directory iterator: %s", strerror(errno));
	}
	return CLOD_REGION_OK;
}
enum clod_region_result dir_iter_next(const dir_iter iter, const char **name) {
	auto const dirp = (DIR *)iter;

next_entry:
	errno = 0;
	struct dirent *ent = readdir(dirp);
	if (!ent) {
		if (errno == 0) {
			*name = nullptr;
			return CLOD_REGION_OK;
		}
		return region_error(CLOD_REGION_INVALID_USAGE, "Reading directory entry: %s", strerror(errno));
	}

#ifdef _DIRENT_HAVE_D_TYPE
	if (ent->d_type != DT_UNKNOWN) {
		if (ent->d_type != DT_REG) goto next_entry;
		*name = ent->d_name;
		return CLOD_REGION_OK;
	}
#endif
	const int fd = dirfd(dirp);
	if (fd < 0) {
		return region_error(CLOD_REGION_INVALID_USAGE, "Failed to get dirfd");
	}
	struct stat st;
	if (fstatat(fd, ent->d_name, &st, 0) != 0) {
		return region_error(CLOD_REGION_INVALID_USAGE, "Failed to stat file %s: %s", ent->d_name, strerror(errno));
	}
	if (!S_ISREG(st.st_mode)) goto next_entry;

	*name = ent->d_name;
	return CLOD_REGION_OK;
}
