#include <assert.h>

#include "../platform.h"
#include "../../error.h"
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

struct file {
	void *map;
	size_t size;
	int fd;
	bool writeable;
};

enum clod_region_result file_open(file *f, const dir d, const char *name, const bool create, const struct clod_region_opts *opts) {
	int o_flags = 0;
	if (opts->mode == CLOD_REGION_MODE_RDWR) o_flags |= O_RDWR;
	if (opts->mode == CLOD_REGION_MODE_RDONLY) o_flags |= O_RDONLY;
	if (create) o_flags |= O_CREAT;

	mode_t o_mode = 0664;
	if (opts->unix_file_perms) o_mode = opts->unix_file_perms;

	const int fd = openat((int)(intptr_t)d, name, o_flags, o_mode);
	if (fd < 0) {
		if (!create && errno == ENOENT) {
			return CLOD_REGION_NOT_FOUND;
		}

		region_error(CLOD_REGION_INVALID_USAGE, "Opening \"%s\": %s", name, strerror(errno));
		return CLOD_REGION_INVALID_USAGE;
	}

#if HAVE_STATX
	struct statx st;
	if (statx(fd, "", AT_EMPTY_PATH, STATX_SIZE, &st)) {
		region_error(CLOD_REGION_INVALID_USAGE, "Failed to stat \"%s\": %s", name, strerror(errno));
		close(fd);
		return CLOD_REGION_INVALID_USAGE;
	}
	const size_t size = st.stx_size;
#else
	struct stat st;
	if (fstat(fd, &st)) {
		region_error(CLOD_REGION_INVALID_USAGE, "Failed to stat \"%s\": %s", name, strerror(errno));
		close(fd);
		return CLOD_REGION_INVALID_USAGE;
	}
	const size_t size = (size_t)st.st_size;
#endif

	int prot = PROT_READ;
	if (opts->mode == CLOD_REGION_MODE_RDWR) prot |= PROT_WRITE;
	void *map = nullptr;
	if (size > 0) {
		map = mmap(nullptr, size, prot, MAP_SHARED, fd, 0);
		if (map == MAP_FAILED) {
			region_error(CLOD_REGION_INVALID_USAGE, "Failed to mmap \"%s\": %s", name, strerror(errno));
			close(fd);
			return CLOD_REGION_INVALID_USAGE;
		}
	}

	struct file *file_struct = malloc(sizeof(struct file));
	if (!file_struct) {
		region_error(CLOD_REGION_INVALID_USAGE, "Failed to allocate memory for file.");
		if (size > 0) munmap(map, size);
		close(fd);
		return CLOD_REGION_INVALID_USAGE;
	}

	file_struct->map = map;
	file_struct->size = size;
	file_struct->fd = fd;
	file_struct->writeable = opts->mode == CLOD_REGION_MODE_RDWR;
	*f = (uintptr_t)file_struct;
	return CLOD_REGION_OK;
}
enum clod_region_result file_get(const file f, void **data, size_t *size) {
	*data = ((struct file *)f)->map;
	*size = ((struct file *)f)->size;
	return CLOD_REGION_OK;
}
enum clod_region_result file_truncate(const file f, const size_t new_size) {
	auto const file_struct = (struct file *)f;
	if (file_struct->size == new_size) return CLOD_REGION_OK;

	if (ftruncate(file_struct->fd, (off_t)new_size)) {
		return region_error(CLOD_REGION_INVALID_USAGE, "Failed to truncate file: %s", strerror(errno));
	}

	const size_t old_size = file_struct->size;
	file_struct->size = new_size;

#if HAVE_MREMAP
	if (file_struct->map != nullptr && new_size > 0) {
		assert(old_size > 0);
		file_struct->map = mremap(file_struct->map, old_size, new_size, MREMAP_MAYMOVE);
		if (file_struct->map == MAP_FAILED) {
			file_struct->map = nullptr;
			return region_error(CLOD_REGION_INVALID_USAGE, "Failed to remap file: %s", strerror(errno));
		}
		return CLOD_REGION_OK;
	}
#endif

	if (file_struct->map != nullptr) {
		assert(old_size > 0);
		munmap(file_struct->map, old_size);
		file_struct->map = nullptr;
	}
	if (new_size > 0) {
		int mmap_flag = PROT_READ;
		if (file_struct->writeable) mmap_flag |= PROT_WRITE;

		file_struct->map = mmap(nullptr, new_size, mmap_flag, MAP_SHARED, file_struct->fd, 0);
		if (file_struct->map == MAP_FAILED) {
			file_struct->map = nullptr;
			return region_error(CLOD_REGION_INVALID_USAGE, "Failed to map file: %s", strerror(errno));
		}
	}
	return true;
}
enum clod_region_result file_close(const file f) {
	auto const file_struct = (struct file *)f;
	enum clod_region_result res = CLOD_REGION_OK;
	if (file_struct->map && munmap(file_struct->map, file_struct->size)) {
		res = region_error(CLOD_REGION_INVALID_USAGE, "Failed to unmap file: %s", strerror(errno));
	}
	if (close(file_struct->fd)) {
		res = region_error(CLOD_REGION_INVALID_USAGE, "Failed to close file: %s", strerror(errno));
	}
	free(file_struct);
	return res;
}
