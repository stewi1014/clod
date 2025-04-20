#ifdef __linux__ 

#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "region.h"

struct RG_File {
    char *data;
    char *path;
    long size;
};

struct RG_File RG_open(char *path) {
    struct RG_File file;
    file.data = NULL;
    file.path = malloc(strlen(path) + 1); strcpy(file.path, path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "failed to open file \"%s\": %s\n", path, strerror(errno));
        goto done;
    }

    struct stat st;
    if (fstat(fd, &st)) {
        fprintf(stderr, "failed to stat \"%s\": %s\n", path, strerror(errno));
        goto close;
    }

    file.size = st.st_size;

    file.data = mmap(NULL, file.size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file.data == MAP_FAILED) {
        fprintf(stderr, "failed to mmap \"%s\": %s\n", path, strerror(errno));
        goto close;
    }

close:
    if (close(fd))
        fprintf(stderr, "failed to close \"%s\": %s\n", path, strerror(errno));

done:
    return file;
}

void RG_close(struct RG_File file) {
    if (munmap(file.data, file.size))
        fprintf(stderr, "failed to unmap memory for \"%s\": %s\n", file.path, strerror(errno));

    free(file.path);
}

#elif _WIN32
    // windows code goes here
#elif __APPLE__

#else
#error Platform not supported (only Linux, Windows and OSX are supported at the moment)
#endif
