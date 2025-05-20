#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <inttypes.h>

#include "anvil.h"

#include <stdio.h>

#include "anvil_internal.h"

const char *anvil_strerror(const anvil_result res) {
    switch (res) {
    case ANVIL_OK: return "Ok";
    case ANVIL_TOO_SMALL: return "Insufficient space";
    case ANVIL_MALFORMED: return "Malformed";
    case ANVIL_ERROR_IO: return "IO Error";
    case ANVIL_INVALID_USAGE: return "Invalid usage";
    case ANVIL_ALLOC_FAILED: return "Alloc failed";
    case ANVIL_LOCKED: return "Locked";
    case ANVIL_NOT_EXIST: return "Does not exist";
    case ANVIL_DONE: return "Done";
    case ANVIL_UNSUPPORTED_COMPRESSION: return "Unsupported compression";
    case ANVIL_OTHER: return "Other error";
    case ANVIL_NO_SPACE: return "No space";
    default: return "Invalid anvil_result";
    }
}

int anvil_message_default_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    int i = 0;

    int ret = vfprintf(stderr, format, args);
    va_end(args);
    if (ret < 0) {
        return ret;
    }

    i += ret;

    ret = fputc('\n', stderr);
    if (ret < 0) {
        return ret;
    }

    i += ret;
    return i;
}



/** the default allocator. if a null custom allocator is given, this is what is used. */
const anvil_allocator default_anvil_allocator = {malloc, free, realloc};

_Thread_local anvil_result anvil_errno_value = ANVIL_OK;
anvil_result *__anvil_errno_location() { return &anvil_errno_value; }
anvil_result anvil_get_error() { return anvil_errno_value; }
void anvil_set_error(const anvil_result result) { anvil_errno_value = result; }

// https://utf8everywhere.org/

const char *prev_dot(const char *str, const char *end) {
    anvil_assert(str != nullptr, return nullptr);
    anvil_assert(end != nullptr, return nullptr);

    while (end > str) {
        end--;
        if (*end == '.') return end;
        if (*end == CLOD_PATH_SEP) return nullptr;
    }
    return nullptr;
}


/**
 * returns true if the space between (not including) the two pointers contains a valid coordinate.
 *
 * To make usage a bit more foolproof the order of arguments doesn't matter,
 * and it returns false if not surrounded by '.'.
 */
bool is_valid_coordinate(const char *str1, const char *str2, int64_t *n) {
    if (str1 == nullptr || str2 == nullptr) return false;

    if (str1 > str2) {
        const char *tmp = str1;
        str1 = str2;
        str2 = tmp;
    }

    if (*str1 != '.' || *str2 != '.') return false;
    if (str2 - str1 < 2) return false;

    char *end;
    *n = strtoll(str1 + 1, &end, 10);
    return end == str2;
}

int64_t anvil_parse_vfilename(
    const char *name,
    const size_t name_size,
    size_t *prefix_size_ptr,
    const char **extension_ptr,
    const int64_t num_coords,
    int64_t *coords
) {
    const char *dot = prev_dot(name, name + name_size);
    if (dot == nullptr) {
        // couldn't even find a '.'.
        if (prefix_size_ptr != nullptr) *prefix_size_ptr = name_size;
        if (extension_ptr != nullptr) *extension_ptr = name + name_size;
        return 0;
    }

next_dot:
    const char *next = prev_dot(name, dot);
    if (next == nullptr) {
        // could not find second '.'.
        // no valid coordinates.
        if (prefix_size_ptr != nullptr) *prefix_size_ptr = dot - name;
        if (extension_ptr != nullptr) *extension_ptr = dot + 1;
        return 0;
    }

    int64_t coord;
    if (!is_valid_coordinate(next, dot, &coord)) {
        dot = next;
        goto next_dot;
    }

    if (extension_ptr != nullptr) *extension_ptr = dot + 1;
    int64_t i = 0;

    do {
        if (coords != nullptr && i < num_coords) {
            coords[num_coords - i - 1] = coord;
        }
        i++;
        dot = next;
    } while (
        (next = prev_dot(name, next)) != nullptr &&
        is_valid_coordinate(next, dot, &coord)
    );

    if (prefix_size_ptr != nullptr) *prefix_size_ptr = dot - name;
    return i;
}

int64_t anvil_parse_filename(
    const char *name,
    const size_t name_size,
    size_t *prefix_size_ptr,
    const char **extension_ptr,
    int64_t num_coords,
    ... /** int64_t *coord... */
) {
    int64_t arr[num_coords == 0 ? 1 : num_coords];

    const auto res = anvil_parse_vfilename(name, name_size, prefix_size_ptr, extension_ptr, num_coords, arr);

    va_list va;
    va_start(va, num_coords);

    for (int64_t i = 0; i < num_coords; ++i) {
        *va_arg(va, int64_t*) = arr[i];
    }

    va_end(va);
    return res;
}

size_t anvil_create_vfilename(
    char *name,
    const size_t name_size,
    const char *prefix,
    const char *extension,
    const int64_t num_coords,
    const int64_t *coords
) {
    size_t n = 0;
    #define write(c) {if (name != nullptr && n < name_size) { name[n++] = (c); } else { (void)(c); n++; }}

    if (prefix != nullptr) while (*prefix != '\0') write(*prefix++);

    for (int64_t i = 0; i < num_coords; ++i) {
        write('.');
        n += snprintf(
            name != nullptr && n < name_size ? name + n : nullptr,
            name != nullptr && n < name_size ? name_size - n : 0,
            "%" PRId64,
            coords[i]
        );
    }
    write('.');

    if (extension != nullptr) while (*extension != '\0') write(*extension++);

    write('\0');

    #undef write
    return n;
}

size_t anvil_create_filename(
    char *name,
    const size_t name_size,
    const char *prefix,
    const char *extension,
    int64_t num_coords,
    ... /** int64_t coord... */
) {
    int64_t arr[num_coords == 0 ? 1 : num_coords];

    va_list va;
    va_start(va, num_coords);

    for (int64_t i = 0; i < num_coords; ++i) {
        arr[i] = va_arg(va, int64_t);
    }

    va_end(va);

    return anvil_create_vfilename(name, name_size, prefix, extension, num_coords, arr);
}
