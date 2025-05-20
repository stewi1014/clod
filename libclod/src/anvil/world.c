#include "world.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>



#ifdef CLOD_USE_POSIX
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
        anvil_set_result(ANVIL_ALLOC_FAILED, nullptr);
        return nullptr;
    } else {
        a = *alloc;
    }

    #ifdef CLOD_USE_POSIX

    const int dir_fd = open(path, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        return nullptr;
    }

    const int session_lock_fd = openat(dir_fd, "session.lock", O_WRONLY | O_CREAT | O_TRUNC);
    if (session_lock_fd == -1) {
        const auto err = errno;
        close(dir_fd);
        errno = err;
        return nullptr;
    }

    if (write(session_lock_fd, SESSION_LOCK_STR, SESSION_LOCK_STR_LEN) < SESSION_LOCK_STR_LEN) {
        const auto err = errno;
        close(session_lock_fd);
        close(dir_fd);
        errno = err;
        return nullptr;
    }

    if (fsync(session_lock_fd)) {
        const auto err = errno;
        close(session_lock_fd);
        close(dir_fd);
        errno = err;
        return nullptr;
    }

    #else
    #error not implemented
    #endif

    anvil_world *world = malloc(sizeof(anvil_world));
    if (!world) {
        close(session_lock_fd);
        close(dir_fd);
        return nullptr;
    }
    *world = (anvil_world){
        .alloc = a,

        #ifdef CLOD_USE_POSIX
        .dir_fd = dir_fd,
        .session_lock_fd = session_lock_fd,
        #else
        #error not implemented
        #endif
    };

    return world;
}

void anvil_close(anvil_world *world) {
    #ifdef CLOD_USE_POSIX

    close(world->session_lock_fd);
    close(world->dir_fd);

    #else
        #error not implemented
    #endif

    world->alloc.free(world);
}

anvil_dir *anvil_world_open_dir(
    const anvil_world *world,
    const char *dir,
    const int64_t num_coordinates,
    const size_t section_size,
    const char *region_extension,
    const char *chunk_extension
) {
    #ifdef CLOD_USE_POSIX
    const int dir_fd = openat(world->dir_fd, dir, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        return nullptr;
    }

    anvil_dir *res = anvil_dir_new(
        &world->alloc, dir_fd, num_coordinates, section_size, region_extension, chunk_extension
    );

    if (res == nullptr) {
        const auto err = errno;
        close(dir_fd);
        errno = err;
        return nullptr;
    }

    return res;

    #else
    #error not implemented
    #endif
}
