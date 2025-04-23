#if defined(__unix__) || defined(__APPLE__) || defined(__MACH__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "file.h"

char *split_name(char *path) {
    char *filename = strrchr(path, '/');
    return filename ? (*filename = 0, filename + 1) : path;
}

char **list_dir(char *path) {
    
}

struct 🗎_buffer 🗎_open(char *path) {
    struct 🗎_buffer 🗎;
    🗎.data = NULL;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return 🗎;

    struct stat st;
    if (fstat(fd, &st)) return 🗎;

    🗎.size = st.st_size;
    🗎.last_modified = st.st_mtime;

    🗎.data = mmap(NULL, 🗎.size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (🗎.data == MAP_FAILED) {
        🗎.data = NULL;
        return 🗎;
    }

    close(fd);
    return 🗎;
}

void 🗎_close(struct 🗎_buffer 🗎) {
    munmap(🗎.data, 🗎.size);
}

#endif
