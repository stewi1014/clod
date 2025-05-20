#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "anvil.h"
#include "path.h"

#define JOIN(...) path_join(&world->tmp_string, &world->tmp_string_cap, world->alloc, __VA_ARGS__)

/// @private
struct anvil_world {
    char *path;

    char *tmp_string;
    size_t tmp_string_cap;

    FILE *session_lock;

    const anvil_allocator *alloc;
};

anvil_result anvil_world_open(
    struct anvil_world **world_out,
    const char *path,
    const anvil_allocator *alloc
) {
    if (path == nullptr || world_out == nullptr) return ANVIL_INVALID_ARGUMENT;
    if (alloc == nullptr)
        alloc = &default_anvil_allocator;
    else if (
        alloc->malloc == nullptr ||
        alloc->calloc == nullptr ||
        alloc->free == nullptr ||
        alloc->realloc == nullptr
    ) {
        return ANVIL_INVALID_ARGUMENT;
    }

    size_t path_len = strlen(path);
    if (path_len == 0) {
        return ANVIL_NOT_EXIST;
    }

    if (
        path_len >= strlen(PATH_SEP"level.dat") &&
        strcmp(path + path_len - strlen(PATH_SEP"level.dat"), PATH_SEP"level.dat") == 0
    ) {
        path_len -= strlen(PATH_SEP"level.dat");
    }

    char *path_copy = alloc->malloc(path_len + 1);
    if (path_copy == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }
    memcpy(path_copy, path, path_len - 1);
    path_copy[path_len] = '\0';

    struct anvil_world *world = alloc->malloc(sizeof(struct anvil_world));
    if (world == nullptr) {
        alloc->free(path_copy);
        return ANVIL_ALLOC_FAILED;
    }

    world->path = path_copy;
    world->tmp_string = nullptr;
    world->tmp_string_cap = 0;
    world->session_lock = nullptr;
    world->alloc = alloc;

    const char *session_lock_path = JOIN(world->path, PATH_SEP, "session.lock", nullptr);
    if (session_lock_path == nullptr) {
        world->alloc->free(world->path);
        world->alloc->free(world);
        return ANVIL_ALLOC_FAILED;
    }

    world->session_lock = fopen(session_lock_path, "wb");
    if (world->session_lock == nullptr) {
        world->alloc->free(world->tmp_string);
        world->alloc->free(world->path);
        world->alloc->free(world);
        return ANVIL_IO_ERROR;
    }

    if (fputs("C", world->session_lock) < 0) {
        fclose(world->session_lock);
        world->alloc->free(world->tmp_string);
        world->alloc->free(world->path);
        world->alloc->free(world);
        return ANVIL_IO_ERROR;
    }

    if (fflush(world->session_lock)) {
        fclose(world->session_lock);
        world->alloc->free(world->tmp_string);
        world->alloc->free(world->path);
        world->alloc->free(world);
        return ANVIL_IO_ERROR;
    }

    #ifndef _WIN32
        #include <unistd.h>
        if (lockf(fileno(world->session_lock), F_TLOCK, -ftell(world->session_lock))) {
            anvil_result res = ANVIL_IO_ERROR;
            if (errno == EACCES || errno == EAGAIN) res = ANVIL_LOCKED;
            fclose(world->session_lock);
            world->alloc->free(world->tmp_string);
            world->alloc->free(world->path);
            world->alloc->free(world);
            return res;
        }
    #endif

    *world_out = world;
    return ANVIL_OK;
}

anvil_result anvil_world_open_region_dir(
    struct anvil_region_dir **region_dir_out,
    struct anvil_world *world,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension
) {
    if (world == nullptr) return ANVIL_INVALID_ARGUMENT;
    return anvil_region_dir_open(
        region_dir_out,
        JOIN(world->path, PATH_SEP, subdir, nullptr),
        region_extension,
        chunk_extension,
        world->alloc
    );
}

void anvil_world_close(struct anvil_world *world) {
    if (world == nullptr) return;
    fclose(world->session_lock);
    world->alloc->free(world->tmp_string);
    world->alloc->free(world->path);
    world->alloc->free(world);
}
