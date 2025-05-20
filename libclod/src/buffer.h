/**
* @private @file
 * I'm gonna reimplement the ENTIRE stdlib with custom memory allocation!!!
 */

#pragma once

#include <stdarg.h>
#include <string.h>

/** returns a copy of the first non-null, non-empty string. */
char *string_copy(
    void*(*malloc_f)(size_t),
    const char *str,
    ...
);

/** concatenates the given strings into tmp_string, growing it using realloc_f as needed. */
char *string_concat(
    char **tmp_string,
    size_t *tmp_string_cap,
    void*(*realloc_f)(void *, size_t),
    const char *elem,
    ...
);
