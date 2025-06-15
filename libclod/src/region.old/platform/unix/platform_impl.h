#ifndef PLATFORM_IMPL_H
#define PLATFORM_IMPL_H

#include <dirent.h>
#include <stddef.h>

struct dir {
	int fd;
};

struct dir_iter {
	DIR *dirp;
	int fd;
};

struct file {
	int fd;
	void *map;
	size_t size;
};

#endif
