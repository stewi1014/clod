#pragma once

#include <stdarg.h>
#include <string.h>

/**
 * I'm gonna reimplement the ENTIRE stdlib with custom memory allocation!!!
 */

// returns a copy of the first non-null, non-empty string.
static char *string_copy(void*(*malloc_f)(size_t), const char *str...) {
    va_list va = nullptr;
    va_start(va, str);

    while (str == nullptr || strlen(str) == 0) {
        str = va_arg(va, const char *);
    }

    const size_t len = strlen(str);
    char *new = malloc_f(len + 1);
    if (new == nullptr) return nullptr;

    return memcpy(new, str, len + 1);
}

// concatenates the given strings into tmp_string, growing it using realloc_f as needed.
static char *string_concat(char **tmp_string, size_t *tmp_string_cap, void*(*realloc_f)(void *, size_t), const char *elem ...) {
    va_list va = nullptr;
    va_start(va, elem);

    size_t size = 0;
    while (elem != nullptr) {
        const size_t len = strlen(elem);
        size += len;
        if (*tmp_string_cap < size + 1) {
            char *new = realloc_f(*tmp_string, size + 1);
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


