#pragma once
#include <time.h>

char *split_name(char *path);

// pass strings and array to free() when done.
char **list_dir(char *path);

struct 🗎_buffer {
    char *data;
    int size;
    time_t last_modified;
};

struct 🗎_buffer 🗎_open(char *path);
void 🗎_close(struct 🗎_buffer);
