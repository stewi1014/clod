#include <stdlib.h>
#include <errno.h>

#include "dir.h"

#include <string.h>

#ifdef CLOD_USE_POSIX
#include <unistd.h>
#include <fcntl.h>
#else
#error not implemented
#endif

#define default_extent(coord_count) (size_t)(\
    (coord_count) == 0 ? 1      :\
    (coord_count) == 1 ? 1024   :\
    (coord_count) == 2 ? 32     :\
    (coord_count) == 3 ? 11     :\
    (coord_count) == 4 ? 6      :\
    (coord_count) == 5 ? 4      :\
    (coord_count) == 6 ? 3      :\
    (coord_count) == 7 ? 2      :\
    (coord_count) == 8 ? 2      :\
    (coord_count) == 9 ? 2      :\
    (coord_count) == 10 ? 2     :\
    0\
)

anvil_dir *anvil_open_dir(
    const anvil_allocator *alloc,
    const char *path,
    const int64_t num_coordinates,
    const size_t section_size,
    const char *region_extension,
    const char *chunk_extension
) {
    #ifdef CLOD_USE_POSIX
    const int dir_fd = open(path, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        return nullptr;
    }

    anvil_dir *res = anvil_dir_new(
        alloc, dir_fd, num_coordinates, section_size, region_extension, chunk_extension
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

anvil_dir *anvil_dir_new(
    const anvil_allocator *alloc,

    #ifdef CLOD_USE_POSIX
    const int dir_fd,
    #else
    #error not implemented
    #endif

    const int64_t num_coordinates,
    const size_t section_size,
    const char *region_extension,
    const char *chunk_extension
) {
    anvil_allocator a;

    if (alloc == nullptr) {
        a.malloc = malloc;
        a.calloc = calloc;
        a.realloc = realloc;
        a.free = free;
    } else {
        a = *alloc;
    }

    size_t region_extension_size = 0;
    if (region_extension) region_extension_size = strlen(region_extension) + 1;

    size_t chunk_extension_size = 0;
    if (chunk_extension) chunk_extension_size = strlen(chunk_extension) + 1;

    // hahaha. I freaking love C.
    anvil_dir *dir = a.malloc(sizeof(anvil_dir) + region_extension_size + chunk_extension_size);
    if (!dir) return nullptr;

    dir->region_extension = region_extension_size == 0 ? "mca" : strncpy(dir->ext, region_extension, region_extension_size);
    dir->chunk_extension = chunk_extension_size == 0 ? "mcc" : strncpy(dir->ext + region_extension_size, chunk_extension, chunk_extension_size);
    dir->layout.count = num_coordinates;
    dir->layout.extent = default_extent(num_coordinates);
    dir->section_size = section_size;

    dir->alloc = a;

    #ifdef CLOD_USE_POSIX
    dir->dir_fd = dir_fd;
    #else
    #error not implemented
    #endif

    return dir;
}

bool anvil_close_dir(anvil_dir *dir) {
    #ifdef CLOD_USE_POSIX
    close(dir->dir_fd);
    #else
    #error not implemented
    #endif

    dir->alloc.free(dir);
}

struct anvil_file *get_file(anvil_dir *dir, const int64_t *chunk) {
    if (dir->file) {
        if (region_contains_chunk(chunk, dir->file->region, dir->layout)) {
            return dir->file;
        }

        anvil_close_file(dir->file);
        dir->file = nullptr;
    }


}

time_t anvil_mtime(anvil_dir *dir, int64_t *coordinates) {

}

uint8_t *anvil_read(
    anvil_dir *dir,
    size_t *n_bytes_read,
    int64_t *coordinates
) {
    if (!dir) return nullptr;
}

bool anvil_write(
    anvil_dir *dir,
    uint8_t *buf,
    size_t n_bytes,
    int64_t *coordinates
) {
    if (!dir) return -1;
}