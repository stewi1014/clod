#include <stdarg.h>
#include <stdlib.h>

#include "anvil.h"

anvil_result anvil_parse_name(
    const char *name,
    int64_t *x_out,
    int64_t *z_out,
    char **extension
) {
    const size_t name_len = strlen(name);
    if (name_len == 0) {
        return ANVIL_INVALID_USAGE;
    }

    // general idea is to start at the end of the string,
    // and keep moving down it until we find two consecutive '.'
    // that have a parsable integer following them.
    char *end_ptr;

    int i = (int)name_len - 2;
    while (i >= 0 && name[i] != '.') i--;
    if (i < 0) return ANVIL_INVALID_USAGE;

    next:
        const int j = i;
    i--;
    while (i >= 0 && name[i] != '.') i--;
    if (i < 0) return ANVIL_INVALID_USAGE;

    errno = 0;
    const auto x = strtoll(name + i + 1, &end_ptr, 10);
    if (x_out != nullptr) *x_out = x;
    if (errno != 0 || end_ptr == name + i + 1) goto next;
    const auto z = strtoll(name + j + 1, &end_ptr, 10);
    if (z_out != nullptr) *z_out = z;
    if (errno != 0 || end_ptr == name + j + 1) goto next;

    if (extension != nullptr)
        *extension = end_ptr + 1;

    return ANVIL_OK;
}

int anvil_message_default_printf(const char *format, ...) {
    va_list args = nullptr;
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

int (anvil_message_printf*)(const char *format, ...) = anvil_message_default_printf;

/** the default allocator. if a null custom allocator is given, this is what is used. */
const anvil_allocator default_anvil_allocator = {malloc, calloc, free, realloc};

_Thread_local anvil_result anvil_errno_value;
anvil_result anvil_get_error() { return anvil_errno_value; }
void anvil_set_error(const anvil_result result) { anvil_errno_value = result; }
