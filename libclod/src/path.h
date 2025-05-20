#pragma once

#include <stdarg.h>
#include <string.h>

#include "clod.h"

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

static char *path_join(char **tmp_string, size_t *tmp_string_cap, const clod_allocator *alloc, const char *elem ...) {
    va_list va = nullptr;
    va_start(va, elem);

    size_t size = 0;
    while (elem != nullptr) {
        const size_t len = strlen(elem);
        size += len;
        if (*tmp_string_cap < size + 1) {
            char *new = alloc->realloc(*tmp_string, size + 1);
            if (new == nullptr) return nullptr;

            *tmp_string = new;
            *tmp_string_cap = size;
        }

        memcpy(*tmp_string + size, elem, len);
        elem = va_arg(va, char *);
    }
    (*tmp_string)[size] = '\0';

    va_end(va);
    return *tmp_string;
}
