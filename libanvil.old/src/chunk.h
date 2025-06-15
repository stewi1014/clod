#pragma once
#include "config.h"
#include <anvil.h>

typedef struct chunk chunk;
#define sizeof_chunk(opts) (sizeof(chunk) + sizeof(int64_t) * (opts).coord_count)
struct chunk {
    uint8_t *data;
    size_t data_size;
    int64_t chunk_pos[];
};
