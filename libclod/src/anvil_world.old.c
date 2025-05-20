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
    const anvil_allocator *alloc;

#ifdef CLOD_USE_POSIX
    int dir_fd;
    int session_lock_fd;
#else
#error not implemenete
#endif
};

anvil_world *anvil_open(
    const char *path,
    const anvil_allocator *alloc
) {
    anvil_assert(path != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);

    if (alloc == nullptr)
        alloc = &default_anvil_allocator;

    anvil_assert(alloc->malloc != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);
    anvil_assert(alloc->calloc != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);
    anvil_assert(alloc->free != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);
    anvil_assert(alloc->realloc != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);

    anvil_world *world = alloc->malloc(sizeof(struct anvil_world));
    if (world == nullptr) {
        anvil_errno = ANVIL_ALLOC_FAILED;
        return nullptr;
    }

    world->alloc = alloc;

#ifdef CLOD_USE_POSIX

    world->dir_fd = open(path, O_DIRECTORY);
    if (world->dir_fd < 0) {
        anvil_errno = anvil_errno_get(errno);
        return nullptr;
    }

    world->session_lock_fd = openat(world->dir_fd, "session.lock", O_WRONLY | O_CREAT | O_TRUNC);
    if (world->session_lock_fd < 0) {
        const auto err = errno;
        close(world->dir_fd);
        anvil_errno = anvil_errno_get(err, errno = err);
        return nullptr;
    }

    if (lockf(world->session_lock_fd, F_TLOCK, 0)) {
        const auto err = errno;
        close(world->dir_fd);
        close(world->session_lock_fd);
        errno = err;

        if (err == EACCES || err == EAGAIN) {
            anvil_errno = ANVIL_LOCKED;
            return nullptr;
        }
        anvil_errno = anvil_errno_get(err, errno = err);
        return nullptr;
    }

    const ssize_t w = write(world->session_lock_fd, SESSION_LOCK_STR, SESSION_LOCK_STR_LEN);
    if (w < SESSION_LOCK_STR_LEN) {
        const auto err = errno;
        close(world->dir_fd);
        close(world->session_lock_fd);
        anvil_errno = anvil_errno_get(err, errno = err);
        return nullptr;
    }

    if (fsync(world->session_lock_fd)) {
        const auto err = errno;
        close(world->dir_fd);
        close(world->session_lock_fd);
        anvil_errno = anvil_errno_get(err, errno = err);
        return nullptr;
    }

#else
#error not implemented
#endif

    return world;
}

anvil_dir *anvil_open_dir(
    const anvil_world *world,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension
) {
    anvil_assert(world != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);
    return anvil_region_dir_openat(
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

void anvil_close(anvil_world *world) {
    anvil_assert(world != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return);

#ifdef CLOD_USE_POSIX

    if (close(world->dir_fd) && anvil_errno == ANVIL_OK) {
        anvil_errno = anvil_errno_get(errno);
    }

    if (close(world->session_lock_fd) && anvil_errno == ANVIL_OK) {
        anvil_errno = anvil_errno_get(errno);
    }

#else
#error not implemented
#endif

    world->alloc->free(world);
}
