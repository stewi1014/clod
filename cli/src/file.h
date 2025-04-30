#pragma once
#include <time.h>

char *split_name(char *path);

// pass strings and array to free() when done.
char **list_dir(char *path);

struct 📄 {
    char *📦;
    int 📏;
    time_t 📅;
};

struct 📄 📄_open(char *📍);
void 📄_close(struct 📄);
