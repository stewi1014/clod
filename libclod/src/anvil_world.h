#include <stdio.h>
#include <stddef.h>

struct anvil_world {
    char *path;
    FILE *session_lock_file;
    char **(*open_file)(char *path, size_t *size);
    void (*close_file)(char **file);
    void *(*malloc)(size_t);
    void *(*realloc)(void*, size_t);
    void (*free)(void*);
};