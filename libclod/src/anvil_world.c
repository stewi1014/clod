#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "anvil.h"
#include "anvil_internal.h"

#ifdef CLOD_USE_POSIX

#include <fcntl.h>
#include <unistd.h>

#else
#error not implemented
#endif

#define SESSION_LOCK_STR "libclod"
#define SESSION_LOCK_STR_LEN strlen("libclod")

/// @private
struct anvil_world {
    char *tmp_string;
    size_t tmp_string_cap;

    const anvil_allocator *alloc;

#ifdef CLOD_USE_POSIX
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
    __anvil_assert_return(world_out != nullptr);
    __anvil_assert_return(path != nullptr);

    if (alloc == nullptr)
        alloc = &default_anvil_allocator;

    __anvil_assert_return(alloc->malloc != nullptr);
    __anvil_assert_return(alloc->calloc != nullptr);
    __anvil_assert_return(alloc->free != nullptr);
    __anvil_assert_return(alloc->realloc != nullptr);

    struct anvil_world *world = alloc->malloc(sizeof(struct anvil_world));
    if (world == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

#ifdef CLOD_USE_POSIX

    world->dir_fd = open(path, O_DIRECTORY);
    if (world->dir_fd < 0) {
        return __anvil_errno;
    }

    world->session_lock_fd = openat(world->dir_fd, "session.lock", O_WRONLY | O_CREAT | O_TRUNC);
    if (world->session_lock_fd < 0) {
        __anvil_errno_return(close(world->dir_fd));
    }

    if (lockf(world->session_lock_fd, F_TLOCK, 0)) {
        const auto err = errno;
        close(world->dir_fd);
        close(world->session_lock_fd);
        errno = err;

        if (errno == EACCES || errno == EAGAIN) {
            return ANVIL_LOCKED;
        }
        return __anvil_errno;
    }

    const ssize_t w = write(world->session_lock_fd, SESSION_LOCK_STR, SESSION_LOCK_STR_LEN);
    if (w < SESSION_LOCK_STR_LEN) {
        __anvil_errno_return(close(world->dir_fd), close(world->session_lock_fd));
    }

    if (fsync(world->session_lock_fd)) {
        __anvil_errno_return(close(world->dir_fd), close(world->session_lock_fd))
    }

#else
#error not implemented
#endif

    *world_out = world;
    return ANVIL_OK;
}

anvil_result anvil_world_open_dir(
    struct anvil_dir **dir_out,
    const struct anvil_world *world,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension
) {
    __anvil_assert_return(world != nullptr);
    __anvil_assert_return(dir_out != nullptr);

    return anvil_region_dir_openat(
        dir_out,
        subdir,
        region_extension,
        chunk_extension,

#ifdef CLOD_USE_POSIX

        world->dir_fd,

#else
#error not implemented
#endif

        world->alloc
    );
}

void anvil_world_close(struct anvil_world *world) {
    if (world == nullptr) return;

#ifdef CLOD_USE_POSIX

    close(world->dir_fd);
    close(world->session_lock_fd);

#else
#error not implemented
#endif

    world->alloc->free(world->tmp_string);
    world->alloc->free(world);
}
