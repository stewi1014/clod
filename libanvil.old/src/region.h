#pragma once
#include "config.h"
#include <anvil.h>

#include "chunk.h"

typedef struct region region;
#define sizeof_region(opts) (sizeof(region) + vec_size(opts))
struct region {
    uint8_t *data;
    size_t data_size;

    region *next;
    region **prev;

    #if HAVE_UNIX
    int fd;
    #else
    #error not implemented
    #endif

    int64_t region_pos[];
};

#define REGION_CLEAR (region){nullptr}

anvil_result region_reopen(region *region, const anvil_opts *opts);
anvil_result region_close(region *region);

chunk *region_read_open(region *region, const int64_t *chunk_pos, int8_t *compression, size_t *size);
anvil_result region_read_close(region *region, chunk *chunk);

chunk *region_write_open(region *region, const int64_t *chunk_pos, int8_t compression, size_t size);
anvil_result region_write_close(region *region, chunk *chunk);
