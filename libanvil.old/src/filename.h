#pragma once
#include "config.h"
#include <anvil.h>

int64_t anvil_parse_filename(
    const char *name,
    const char **extension_ptr,
    int64_t coord_count,
    int64_t *coords
);

const char *anvil_create_filename(
    const char *prefix,
    const char *extension,
    int64_t coord_count,
    const int64_t *coords
);
