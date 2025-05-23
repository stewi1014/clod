#pragma once
#include <stdlib.h>

#include "anvil.h"

#ifdef NDEBUG

#define __anvil_assert_return(exp) if (!(exp)) { \
    anvil_message_printf("%s() (%s:%d) assertion failed %s", __func__, __FILE__, __LINE__, #exp); \
    return ANVIL_INVALID_USAGE; \
}

#else

#define __anvil_assert_return(exp) if (!(exp)) { \
    anvil_message_printf("%s() (%s:%d) assertion failed %s", __func__, __FILE__, __LINE__, #exp); \
    __builtin_trap(); \
}

#endif

#define __anvil_errno (\
    (errno) == EPERM ? ANVIL_LOCKED :\
    (errno) == EBADF ? ANVIL_INVALID_USAGE :\
    (errno) == ENOENT ? ANVIL_NOT_EXIST :\
    (errno) == EINTR ? ANVIL_ERRNO :\
    (errno) == EIO ? ANVIL_ERROR_IO :\
    (errno) == ENXIO ? ANVIL_NOT_EXIST :\
    (errno) == ENOMEM ? ANVIL_ALLOC_FAILED :\
    (errno) == EACCES ? ANVIL_NOT_EXIST :\
    (errno) == ENOTDIR ? ANVIL_NOT_EXIST :\
    (errno) == EISDIR ? ANVIL_NOT_EXIST :\
    (errno) == EINVAL ? ANVIL_INVALID_USAGE :\
    (errno) == ENFILE ? ANVIL_ALLOC_FAILED :\
    (errno) == EMFILE ? ANVIL_ALLOC_FAILED :\
    (errno) == EFBIG ? ANVIL_INVALID_USAGE :\
    (errno) == ENOSPC ? ANVIL_NO_SPACE :\
    (errno) == EROFS ? ANVIL_NO_SPACE :\
    (errno) == EDQUOT ? ANVIL_NO_SPACE :\
    (errno) == ENAMETOOLONG ? ANVIL_INVALID_USAGE :\
    (anvil_message_printf("%s() (%s:%d) unknown error %d: %s", __func__, __FILE__, __LINE__, errno, strerror(errno)), ANVIL_ERRNO)\
)

#define __anvil_errno_return(...) \
    const auto err = errno;\
    __VA_ARGS__;\
    errno = err;\
    return __anvil_errno;

/** the default allocator. if a null custom allocator is given, this is what is used. */
extern const anvil_allocator default_anvil_allocator;

/**
 * the posix operating systems provide openat and other *at methods
 * to make the kinds of file operations we make here more robust.
 *
 * windows will need to use similar methods to ensure similar guarantees.
 * I suspect that simply opening the directory may be all that's required.
 * I just wonder about directory renaming.
 */

anvil_result anvil_region_dir_openat(
    struct anvil_dir **region_dir_out,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,

#ifdef CLOD_USE_POSIX
    int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc
);

anvil_result anvil_region_file_openat(
    struct anvil_file **region_file_out,
    int64_t region_x,
    int64_t region_z,
    const char *chunk_extension,
    const char *region_extension,

#ifdef CLOD_USE_POSIX
    int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc
);
