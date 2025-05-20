#include <stdio.h>
#include <stddef.h>

struct anvil_world {
    char *path;
    FILE *session_lock_file;
    char **(*open_file)(const char *path, size_t *size);
    void (*close_file)(char **file);
    void *(*realloc)(void*, size_t);
};
