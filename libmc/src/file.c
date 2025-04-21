#include "file.h"

#ifdef __unix__

#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

char *file_name(const char *path) {
    char *filename = strrchr(path, '/');
    filename ? filename + 1 : path;
}

struct file_buffer file_open(char *path) {
    struct file_buffer file;
    file.data = NULL;
    file.path = malloc(strlen(path) + 1); strcpy(file.path, path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        free(file.path);
        return file;
    }

    struct stat st;
    if (fstat(fd, &st)) {
        free(file.path);
        return file;
    }

    file.size = st.st_size;

    file.data = mmap(NULL, file.size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file.data == MAP_FAILED) {
        free(file.path);
        file.data = NULL;
        return file;
    }

    close(fd);
    return file;
}

void file_close(struct file_buffer file) {
    if (munmap(file.data, file.size))
        fprintf(stderr, "failed to unmap memory for \"%s\": %s\n", file.path, strerror(errno));

    free(file.path);
}

#elif _WIN32
#error Windows is not currently supported. // TODO; windows support
#elif __APPLE__
#error OSX is not currently supported. // TODO; OSX support
#else
#error Platform is not currently supported.
#endif
