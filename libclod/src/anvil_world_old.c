#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>

#include "anvil.h"

#include "file.h"
#include "anvil_world.h"

#define MAX_PATH_LEN 4096 // I hate this


//=======//
// World //
//=======//

struct anvil_world *anvil_open(const char *path) {
    return anvil_open_ex(path, nullptr, nullptr, nullptr, nullptr);
}

struct anvil_world *anvil_open_ex(
    const char *path,
    int scandir(
        const char *restrict dirp,
        struct dirent ***restrict namelist,
        typeof(int (const struct dirent *)) *filter,
        typeof(int (const struct dirent **, const struct dirent **)) *comparator
    ),
    char **(*open_file_f)(const char *path, size_t *size),
    void (*close_file_f)(char **file),
    void *(*realloc_f)(void*, size_t)
) {
    if (open_file_f == nullptr) open_file_f = 📤;
    if (close_file_f == nullptr) close_file_f = 📥;
    if (realloc_f == nullptr) realloc_f = realloc;

    struct anvil_world *world = realloc_f(nullptr, sizeof(struct anvil_world));
    if (world == nullptr) {
        return nullptr;
    }

    size_t path_size = strlen(path);
    if (path_size >= strlen("level.dat") && !strcmp(path + path_size - strlen("level.dat"), "level.dat")) {
        path_size -= strlen("level.dat");
    }
    char *world_path = realloc_f(nullptr, MAX_PATH_LEN);
    if (world_path == nullptr) {
        realloc_f(world, 0);
        return nullptr;
    }
    memcpy(world_path, path, path_size);
    world_path[path_size] = '\x0';

    world_path[path_size] = PATH_SEPERATOR;
    strcpy(world_path + path_size + 1, "session.lock");
    FILE *session_lock = fopen(world_path, "w");
    world_path[path_size] = '\x0';
    if (session_lock == nullptr) {
        realloc_f(world_path, 0);
        realloc_f(world, 0);
        return nullptr;
    }

    if (fputs("☃", session_lock) < 0) {
        fclose(session_lock);
        realloc_f(world_path, 0);
        realloc_f(world, 0);
        return nullptr;
    }
    if (fflush(session_lock)) {
        fclose(session_lock);
        realloc_f(world_path, 0);
        realloc_f(world, 0);
        return nullptr;
    }

    #if defined(__unix__) || defined(__APPLE__)
        #include <unistd.h>
        if (lockf(fileno(session_lock), F_LOCK, -ftell(session_lock))) {
            // hmmmm
            // I have half a mind to ignore this - but I won't.
            fclose(session_lock);
            realloc_f(world_path, 0);
            realloc_f(world, 0);
            return nullptr;
        }
    #endif

    world->path = world_path;
    world->open_file = open_file_f;
    world->close_file = close_file_f;
    world->realloc = realloc_f;
    return world;
}

void anvil_close(struct anvil_world *world) {
    world->realloc(world->path, 0);
    world->realloc(world, 0);
}



//========//
// Region //
//========//

struct anvil_region anvil_region_new(
    char *data,
    const size_t data_size,
    const int64_t region_x,
    const int64_t region_z
) {
    assert(data != nullptr);
    assert(data_size >= 8192);

    struct anvil_region region;
    region.data = data;
    region.data_size = data_size;
    region.region_x = region_x;
    region.region_z = region_z;
    return region;
}

struct anvil_region_iter {
    char *path;
    DIR *dir;
    char **previous_file;
    char **(*open_file)(const char *path, size_t *size);
    void (*close_file)(char **file);
    void *(*realloc)(void*, size_t);
};

struct anvil_region_iter *anvil_region_iter_new(const char *subdir, const struct anvil_world *world) {
    if (world == nullptr) return nullptr;

    struct anvil_region_iter *iter = world->realloc(nullptr, sizeof(struct anvil_region_iter));
    if (iter == nullptr) {
        return nullptr;
    }

    char *path = world->realloc(nullptr, MAX_PATH_LEN);
    strcpy(path, world->path);

    if (subdir != nullptr) {
        const size_t len = strlen(path);
        path[len] = PATH_SEPERATOR;
        strcpy(path + len + 1, subdir);
    }

    DIR *dir = opendir(path);

    if (dir == nullptr) {
        world->realloc(iter, 0);
        return nullptr;
    }

    iter->path = path;
    iter->dir = dir;
    iter->previous_file = nullptr;
    iter->open_file = world->open_file;
    iter->close_file = world->close_file;
    iter->realloc = world->realloc;
    return iter;
}

int anvil_region_iter_next(
    struct anvil_region *region,
    struct anvil_region_iter *iter
) {
    assert(region != nullptr);
    if (iter == nullptr) {
        return -1;
    }

    if (iter->previous_file != nullptr) {
        iter->close_file(iter->previous_file);
        iter->previous_file = nullptr;
    }

    const auto path_size = strlen(iter->path);

next_region:
    const struct dirent* ent = readdir(iter->dir);
    if (ent == nullptr) {
        return 1;
    }

    const int scan_count = sscanf(ent->d_name, "r.%ld.%ld.mca", &region->region_x, &region->region_z);
    if (scan_count != 2) {
        goto next_region;
    }

    iter->path[path_size] = PATH_SEPERATOR;
    strcpy(iter->path + path_size + 1, ent->d_name);

    char **file = iter->open_file(iter->path, &region->data_size);
    iter->path[path_size] = '\x0';
    if (file == nullptr) {
        return -1;
    }

    if (region->data_size == 0) {
        iter->close_file(file);
        goto next_region;
    }

    iter->previous_file = file;
    region->data = *file;
    if (region->data == nullptr) {
        return -1;
    }

    return 0;
}

void anvil_region_iter_free(struct anvil_region_iter *iter) {
    if (iter == nullptr) {
        return;
    }

    if (iter->previous_file != nullptr) {
        iter->close_file(iter->previous_file);
    }

    closedir(iter->dir);
    iter->realloc(iter->path, 0);
    iter->realloc(iter, 0);
}
