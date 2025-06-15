#pragma once

#include "anvil.h"
#include "coord.h"

struct anvil_dir {
    /** Human-readable name for the directory. Used for errors and debugging. */
    const char *name;
    /** File extension that region files have. */
    const char *region_extension;
    /** File extension that chunk files have. */
    const char *chunk_extension;
    anvil_coord_layout layout;
    size_t section_size;

    struct anvil_file *file;
    anvil_allocator alloc;

    #ifdef CLOD_POSIX
    int dir_fd;
    #else
    #error not implemented
    #endif

    char __ext[];
};

anvil_dir *anvil_dir_new(
    const char *path,
    const anvil_allocator *alloc,

    #ifdef CLOD_POSIX
    int dir_fd,
    #else
    #error not implemented
    #endif

    int64_t num_coordinates,
    size_t section_size,
    const char *region_extension,
    const char *chunk_extension
);
