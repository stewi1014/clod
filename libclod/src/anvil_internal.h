#pragma once

#include <anvil.h>
#include "os.h"

/**
 * the posix operating systems provide openat and other *at methods
 * to make the kinds of file operations we make here more robust.
 *
 * windows will need to use similar methods to ensure similar guarantees.
 * I suspect that simply opening the directory may be all that's required.
 * I just wonder about directory renaming.
 */

anvil_result anvil_region_dir_openat(
    struct anvil_region_dir **region_dir_out,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,

#ifdef POSIX
    int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc
);

anvil_result anvil_region_file_openat(
    struct anvil_region_file **region_file_out,
    int64_t region_x,
    int64_t region_z,
    const char *chunk_extension,

#ifdef POSIX
    int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc
);
