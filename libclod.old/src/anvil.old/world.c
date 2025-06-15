#include "world.h"
#include "error.h"
#include "dir.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef CLOD_POSIX
#include <fcntl.h>
#include <unistd.h>
#else
#error not implemented
#endif

#define SESSION_LOCK_STR "libclod"
#define SESSION_LOCK_STR_LEN strlen("libclod")

anvil_world *anvil_open(const char *path, const anvil_allocator *alloc) {
    if (path == nullptr) return nullptr;

    anvil_allocator a;
    if (!alloc) {
        a.malloc = malloc;
        a.calloc = calloc;
        a.realloc = realloc;
        a.free = free;
    } else if (
        !alloc->malloc ||
        !alloc->calloc ||
        !alloc->realloc ||
        !alloc->free
    ) {
        anvil_set_result(ANVIL_INVALID_USAGE, "alloc methods cannot be null");
        return nullptr;
    } else {
        a = *alloc;
    }

    anvil_world *world = alloc->malloc(sizeof(anvil_world));
    if (!world) {
        anvil_set_result(ANVIL_INVALID_USAGE, nullptr);
        return nullptr;
    }

    world->alloc = a;

    #ifdef CLOD_POSIX

    const int dir_fd = open(path, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        anvil_set_errno(errno, "Reading directory %s", path);
        alloc->free(world);
        return nullptr;
    }

    const int session_lock_fd = openat(dir_fd, "session.lock", O_WRONLY | O_CREAT | O_TRUNC);
    if (session_lock_fd == -1) {
        anvil_set_errno(errno, "Opening session.lock");
        close(dir_fd);
        alloc->free(world);
        return nullptr;
    }

    if (write(session_lock_fd, SESSION_LOCK_STR, SESSION_LOCK_STR_LEN) < SESSION_LOCK_STR_LEN) {
        anvil_set_errno(errno, "Writing session.lock");
        close(session_lock_fd);
        close(dir_fd);
        alloc->free(world);
        return nullptr;
    }

    if (fsync(session_lock_fd)) {
        anvil_set_errno(errno, "Syncing session.lock");
        close(session_lock_fd);
        close(dir_fd);
        alloc->free(world);
        return nullptr;
    }

    if (lockf(session_lock_fd, F_TLOCK, 0)) {
        if (errno == EACCES || errno == EAGAIN) {
            errno = 0;
            anvil_set_result(ANVIL_LOCKED, "Locking session.lock");
            close(session_lock_fd);
            close(dir_fd);
            alloc->free(world);
            return nullptr;
        }
    }

    world->dir_fd = dir_fd;
    world->session_lock_fd = session_lock_fd;

    #else
    #error not implemented
    #endif

    return world;
}

void anvil_close(anvil_world *world) {
    #ifdef CLOD_USE_POSIX

    if (close(world->session_lock_fd) && anvil_error() == ANVIL_OK) {
        anvil_set_errno(errno, "Closing session.lock");
    }

    if (close(world->dir_fd) && anvil_error() == ANVIL_OK) {
        anvil_set_errno(errno, "Closing world directory");
    }

    #else
        #error not implemented
    #endif

    world->alloc.free(world);
}

anvil_dir *anvil_world_open_dir(
    const anvil_world *world,
    const char *dir,
    const int64_t coord_count,
    const size_t section_size,
    const char *region_extension,
    const char *chunk_extension
) {
    #ifdef CLOD_POSIX
    const int dir_fd = openat(world->dir_fd, dir, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        anvil_set_errno(errno, "Opening region directory %s", dir);
        return nullptr;
    }

    anvil_dir *res = anvil_dir_new(
        &world->alloc, dir_fd, coord_count, section_size, region_extension, chunk_extension
    );

    if (res == nullptr) {
        close(dir_fd);
        return nullptr;
    }

    return res;

    #else
    #error not implemented
    #endif
}
