#pragma once
#include <signal.h>
#include <stdlib.h>

#include "anvil.h"

#ifdef NDEBUG
#define anvil_breakpoint()
#else
    #ifdef _MSC_VER
        #define anvil_breakpoint() __debugbreak()
    #else
        #define anvil_breakpoint() raise(SIGTRAP)
    #endif
#endif

#define anvil_assert(exp, action /** action on failure */) if (!(exp)) { \
    anvil_message_printf("%s() (%s:%d) assertion failed %s", __func__, __FILE__, __LINE__, #exp); \
    anvil_breakpoint(); \
    action; \
}

/**
 * Attempts to translate an errno value into an anvil error.
 */
#define anvil_errno_get(err, ... /** actions on unknown errno */) (\
    err == EPERM ? ANVIL_LOCKED :\
    err == EBADF ? ANVIL_INVALID_USAGE :\
    err == ENOENT ? ANVIL_NOT_EXIST :\
    err == EIO ? ANVIL_ERROR_IO :\
    err == ENXIO ? ANVIL_NOT_EXIST :\
    err == ENOMEM ? ANVIL_ALLOC_FAILED :\
    err == EACCES ? ANVIL_NOT_EXIST :\
    err == ENOTDIR ? ANVIL_NOT_EXIST :\
    err == EISDIR ? ANVIL_NOT_EXIST :\
    err == EINVAL ? ANVIL_INVALID_USAGE :\
    err == ENFILE ? ANVIL_ALLOC_FAILED :\
    err == EMFILE ? ANVIL_ALLOC_FAILED :\
    err == EFBIG ? ANVIL_INVALID_USAGE :\
    err == ENOSPC ? ANVIL_NO_SPACE :\
    err == EROFS ? ANVIL_NO_SPACE :\
    err == EDQUOT ? ANVIL_NO_SPACE :\
    err == ENAMETOOLONG ? ANVIL_INVALID_USAGE :\
    (anvil_message_printf("%s() (%s:%d) unknown error %d: %s", __func__, __FILE__, __LINE__, err, strerror(err)), ##__VA_ARGS__,  ANVIL_OTHER)\
)

/** the default allocator. if a null custom allocator is given, this is what is used. */
extern const anvil_allocator default_anvil_allocator;

/**
 * the posix operating systems provide openat and other *at methods
 * to make the kinds of file operations we make here more robust.
 *
 * i.e. directory is protected from deletion and some other changes,
 * and we don't have to care about its name or path after we first open it.
 *
 * windows will need to use similar methods to ensure similar guarantees.
 * I suspect that simply opening the directory may be all that's required.
 * I just wonder about directory renaming.
 */
anvil_dir *anvil_region_dir_openat(
    const char *subdir,
    size_t region_coordinates,
    const char *region_extension,
    size_t chunk_coordinates,
    const char *chunk_extension,

#ifdef CLOD_USE_POSIX
    int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc
);

anvil_file *anvil_region_file_openat(
    const char *chunk_extension,
    const char *region_extension,

#ifdef CLOD_USE_POSIX
    int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc,
    ... /* int64_t coordinates */
);
