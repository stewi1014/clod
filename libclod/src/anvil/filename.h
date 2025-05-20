#pragma once
#include "anvil.h"
#include "coord.h"

int64_t anvil_parse_filename(
    const char * name,
    const char ** extension_ptr,
    int64_t num_coords,
    int64_t * coords
);

const char *anvil_create_filename(
    const char * prefix,
    const char * extension,
    int64_t num_coords,
    const int64_t * coords
);
