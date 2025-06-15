#pragma once

#ifdef CLOD_POSIX
#include <sys/stat.h>
#else
#error not implemented
#endif

#include <bits/pthreadtypes.h>

#include "anvil.h"
#include "coord.h"

typedef struct anvil_file {
    /** region file */
    uint8_t *file;
    /* shadow header */
    uint8_t *sh;

    int64_t *region;
    size_t size;
    anvil_opts opts;

#ifdef CLOD_POSIX

    int dir_fd;
    int file_fd;

#else
    #error not implemented
#endif

    uint8_t __ext[];
} anvil_file;

anvil_file *anvil_file_open(
    const int64_t *region,
    size_t section_size,
    anvil_allocator alloc,

    #ifdef CLOD_POSIX
    int dir_fd
    #else
    #error not implemented
    #endif
);

anvil_result anvil_close_file(anvil_file *file);
