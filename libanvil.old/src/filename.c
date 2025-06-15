#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *prev_dot(const char *str, const char *end) {
    while (end > str) {
        end--;
        if (*end == '.') return end;
    }
    return nullptr;
}

static bool parse_coord(const char *start_dot, const char *end_dot, int64_t *n) {
    if (end_dot - start_dot < 2) return false;
    char *end;
    *n = strtoll(start_dot + 1, &end, 10);
    return end == end_dot;
}

int64_t anvil_parse_filename(
    const char *const name,
    const char **const extension_ptr,
    const int64_t coord_count,
    int64_t *const coords
) {
    const size_t name_size = strlen(name);
    const char *dot = prev_dot(name, name + name_size);
    if (!dot) return -1;

    next_dot:
        const char *next = prev_dot(name, dot);
    if (!next) return 0;

    int64_t coord;
    if (!parse_coord(next, dot, &coord)) {
        dot = next;
        goto next_dot;
    }

    if (extension_ptr) *extension_ptr = dot + 1;
    int64_t num_parsed;
    do {
        if (coords && num_parsed <= coord_count) coords[coord_count - num_parsed - 1] = coord;
        num_parsed++;
        dot = next;
    } while (
        ((next = prev_dot(name, dot))) &&
        parse_coord(next, dot, &coord)
    );

    if (coords && num_parsed < coord_count)
        for (int64_t i = 0; i < num_parsed; i++) {
            coords[i] = coords[i + coord_count - num_parsed];
        };

    return num_parsed;
}

const char *anvil_create_filename(
    const char *const prefix,
    const char *const extension,
    const int64_t coord_count,
    const int64_t *const coords
) {
    _Thread_local static char name[256];
    #define write(c) {if (n < 255) { name[n++] = (c); } else { (void)(c); n++; }}

    size_t n = 0;
    for (int64_t i = 0; prefix[i] != '\0'; i++) write(prefix[i]);

    for (int64_t i = 0; i < coord_count; i++) {
        write('.');

        n += (size_t)snprintf(
            n < 255 ? name + n : nullptr,
            n < 255 ? 255 - n : 0,
            "%" PRId64,
            coords[i]
        );
    }

    write('.');
    for (int64_t i = 0; extension[i] != '\0'; i++) write(extension[i]);
    write('\0');
    name[255] = '\0'; // Good value-for-money insurance policy.

    #undef write
    return name;
}
