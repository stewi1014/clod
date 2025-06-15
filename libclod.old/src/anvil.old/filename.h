#pragma once
#include "anvil.h"
#include "coord.h"

int64_t anvil_parse_filename(
    const char * name,
    const char ** extension_ptr,
    int64_t *coords,
    coord_layout_t layout
);

const char *anvil_create_filename(
    const char * prefix,
    const char * extension,
    const int64_t *coords,
    coord_layout_t layout
);
