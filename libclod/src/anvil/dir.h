#pragma once

#include "anvil.h"
#include "file.h"
#include "coord.h"

#define MAGIC_VERSION "libclod region v1"

struct anvil_dir {
    const char *region_extension;
    const char *chunk_extension;
    coord_layout_t layout;
    size_t section_size;

    struct anvil_file *file;
    anvil_allocator alloc;

    #ifdef CLOD_USE_POSIX
    int dir_fd;
    #else
    #error not implemented
    #endif

    char ext[];
};

anvil_dir *anvil_dir_new(
    const anvil_allocator *alloc,

    #ifdef CLOD_USE_POSIX
    int dir_fd,
    #else
    #error not implemented
    #endif

    int64_t num_coordinates,
    size_t section_size,
    const char *region_extension,
    const char *chunk_extension
);
