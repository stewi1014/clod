#pragma once
#include "anvil.h"

struct anvil_iter {
    int64_t *chunks;
    size_t chunks_count;
    size_t coords_count;

    size_t index;

    anvil_allocator alloc;
};