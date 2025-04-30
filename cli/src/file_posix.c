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

struct 📄 📄_open(char *📍) {
    struct 📄 📑;
    📑.📦 = NULL;

    int 🖋️ = open(📍, O_RDONLY);
    if (🖋️ < 0) return 📑;

    struct stat 🔎;
    if (fstat(🖋️, &🔎)) return 📑;

    📑.📏 = 🔎.st_size;
    📑.📅 = 🔎.st_mtime;

    📑.📦 = mmap(NULL, 📑.📏, PROT_READ, MAP_PRIVATE, 🖋️, 0);
    if (📑.📦 == MAP_FAILED) {
        📑.📦 = NULL;
        return 📑;
    }

    close(🖋️);
    return 📑;
}

void 📄_close(struct 📄 📑) {
    munmap(📑.📦, 📑.📏);
}

#endif
