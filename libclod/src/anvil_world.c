#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "anvil.h"
#include "anvil_internal.h"
#include "os.h"

#ifdef POSIX
#include <fcntl.h>
#include <unistd.h>
#else
#error not implemented
#endif

#define SESSION_LOCK_STR "libclod"
#define SESSION_LOCK_STR_LEN strlen("libclod")

#define JOIN(...) path_join(&world->tmp_string, &world->tmp_string_cap, world->alloc, __VA_ARGS__)

/// @private
struct anvil_world {
    char *tmp_string;
    size_t tmp_string_cap;

    const anvil_allocator *alloc;

#ifdef POSIX
    int dir_fd;
    int session_lock_fd;
#else
#error not implemenete
#endif
};

anvil_result anvil_world_open(
    struct anvil_world **world_out,
    const char *path,
    const anvil_allocator *alloc
) {
    if (path == nullptr || world_out == nullptr) return ANVIL_INVALID_USAGE;
    if (alloc == nullptr)
        alloc = &default_anvil_allocator;
    else if (
        alloc->malloc == nullptr ||
        alloc->calloc == nullptr ||
        alloc->free == nullptr ||
        alloc->realloc == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

    struct anvil_world *world = alloc->malloc(sizeof(struct anvil_world));
    if (world == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

#ifdef POSIX

    world->dir_fd = open(path, O_DIRECTORY);
    if (world->dir_fd < 0) {
        switch (errno) {
        case ENOENT:
        case ENOTDIR: errno = 0; return ANVIL_NOT_EXIST;
        case EINVAL:
        case ENAMETOOLONG: errno = 0; return ANVIL_INVALID_NAME;
        case ENOMEM: errno = 0; return ANVIL_ALLOC_FAILED;
        default: return ANVIL_IO_ERROR;
        }
    }

    world->session_lock_fd = openat(world->dir_fd, "session.lock", O_WRONLY | O_CREAT | O_TRUNC);
    if (world->session_lock_fd < 0) {
        const auto err = errno;
        close(world->dir_fd);
        errno = err;

        switch (errno) {
        case ENOENT:
        case ENOTDIR: errno = 0; return ANVIL_NOT_EXIST;
        case EINVAL:
        case ENAMETOOLONG: errno = 0; return ANVIL_INVALID_NAME;
        case ENOMEM: errno = 0; return ANVIL_ALLOC_FAILED;
        default: return ANVIL_IO_ERROR;
        }
    }

    if (lockf(world->session_lock_fd, F_TLOCK, 0)) {
        const auto err = errno;
        close(world->dir_fd);
        close(world->session_lock_fd);
        errno = err;

        switch (errno) {
        case EACCES: errno = 0; return ANVIL_LOCKED;
        case ENOLCK: errno = 0; return ANVIL_ALLOC_FAILED;
        default: return ANVIL_IO_ERROR;
        }
    }

    const ssize_t w = write(world->session_lock_fd, SESSION_LOCK_STR, SESSION_LOCK_STR_LEN);
    if (w < SESSION_LOCK_STR_LEN) {
        const auto err = errno;
        close(world->dir_fd);
        close(world->session_lock_fd);
        errno = err;

        switch (errno) {
        case EDQUOT:
        case ENOSPC: errno = 0; return ANVIL_DISK_FULL;
        default: return ANVIL_IO_ERROR;
        }
    }

    if (fsync(world->session_lock_fd)) {
        const auto err = errno;
        close(world->dir_fd);
        close(world->session_lock_fd);
        errno = err;

        switch (errno) {
        case EDQUOT:
        case ENOSPC: errno = 0; return ANVIL_DISK_FULL;
        default: return ANVIL_IO_ERROR;
        }
    }

#else
#error not implemented
#endif

    *world_out = world;
    return ANVIL_OK;
}

anvil_result anvil_world_open_region_dir(
    struct anvil_region_dir **region_dir_out,
    const struct anvil_world *world,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension
) {
    if (
        world == nullptr ||
        region_dir_out == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

    return anvil_region_dir_openat(
        region_dir_out,
        subdir,
        region_extension,
        chunk_extension,

#ifdef POSIX

        world->dir_fd,

#else
#error not implemented
#endif

        world->alloc
    );
}

void anvil_world_close(struct anvil_world *world) {
    if (world == nullptr) return;

#ifdef POSIX

    close(world->dir_fd);
    close(world->session_lock_fd);

#else
#error not implemented
#endif

    world->alloc->free(world->tmp_string);
    world->alloc->free(world);
}
