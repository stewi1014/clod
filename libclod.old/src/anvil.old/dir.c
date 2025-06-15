#include <stdlib.h>
#include <errno.h>

#include "dir.h"

#include <string.h>
#include <linux/limits.h>
#include <sys/mman.h>

#include "error.h"
#include "file.h"
#include "filename.h"

#ifdef CLOD_POSIX
#include <unistd.h>
#include <fcntl.h>
#else
#error not implemented
#endif

#define LOCK_COUNT

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
    const int64_t coord_count,
    const size_t section_size,
    const char *region_extension,
    const char *chunk_extension
) {
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

    #ifdef CLOD_POSIX
    const int dir_fd = open(path, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        return nullptr;
    }

    anvil_dir *res = anvil_dir_new(
        path, &a, dir_fd, coord_count, section_size, region_extension, chunk_extension
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

/** Takes ownership of dir_fd on success. */
anvil_dir *anvil_dir_new(
    const char *name,
    const anvil_allocator *alloc,

    #ifdef CLOD_POSIX
    const int dir_fd,
    #else
    #error not implemented
    #endif

    const int64_t num_coordinates,
    const size_t section_size,
    const char *region_extension,
    const char *chunk_extension
) {
    const size_t name_size = strlen(name) + 1;

    size_t region_extension_size = 0;
    if (region_extension) region_extension_size = strlen(region_extension) + 1;

    size_t chunk_extension_size = 0;
    if (chunk_extension) chunk_extension_size = strlen(chunk_extension) + 1;

    #ifdef CLOD_POSIX

    #else
    #error not implemented
    #endif

    anvil_dir *dir = alloc->malloc(
        sizeof(anvil_dir) +
        region_extension_size +
        chunk_extension_size +
        name_size
    );
    if (!dir) {
        anvil_set_result(ANVIL_ALLOC_FAILED, nullptr);
        return nullptr;
    }

    dir->name = strncpy(
        dir->__ext,
        name,
        name_size
    );
    dir->region_extension = region_extension_size == 0 ? "mca" : strncpy(
        dir->__ext + name_size,
        region_extension,
        region_extension_size
    );
    dir->chunk_extension = chunk_extension_size == 0 ? "mcc" : strncpy(
        dir->__ext + name_size + region_extension_size,
        chunk_extension,
        chunk_extension_size
    );
    dir->layout.count = num_coordinates;
    dir->layout.extent = default_extent(num_coordinates);
    dir->section_size = section_size;

    dir->alloc = *alloc;

    #ifdef CLOD_POSIX
    dir->dir_fd = dir_fd;
    #else
    #error not implemented
    #endif

    return dir;
}

void anvil_close_dir(anvil_dir *dir) {
    #ifdef CLOD_POSIX
    if (close(dir->dir_fd) && anvil_errno == ANVIL_OK) {
        anvil_set_errno(errno, "Closing region %s", dir->name);
    }
    #else
    #error not implemented
    #endif

    dir->alloc.free(dir);
}

anvil_file *get_file(anvil_dir *dir, const int64_t *region) {
    if (dir->file) {
        if (coord_equal(region, dir->file->region, dir->layout)) {
            return dir->file;
        }

        const anvil_result res = anvil_close_file(dir->file);
        if (res != ANVIL_OK) {
            return nullptr;
        }
    }
}

anvil_result load_file(anvil_dir *dir, const int64_t *chunk, const bool create) {
    int64_t region[dir->layout.count];
    chunk_get_region(region, chunk, dir->layout);

    if (dir->file) {
        if (coord_equal(region, dir->file->region, dir->layout)) {
            return ANVIL_OK;
        }

        anvil_close_file(dir->file);
    }

    const char *filename = anvil_create_filename(
        "r", dir->region_extension, region, dir->layout
    );

    #ifdef CLOD_POSIX
    const int fd = openat(dir->dir_fd, filename, (create ? O_CREAT : 0) | O_RDWR, 0644);
    if (fd < 0) {
        if (errno == ENOENT && !create) {
            errno = 0;
            dir->file = nullptr;
            return ANVIL_OK;
        }

        return anvil_set_errno(errno, "Opening region file %s", filename);
    }

    dir->file = anvil_file_new(
        filename,
        dir->layout,
        region,
        dir->section_size,
        dir->alloc,
        fd
    );

    if (!dir->file) {
        return anvil_error();
    }
    return ANVIL_OK;

    #else
    #error not implemented
    #endif
}

uint32_t anvil_mtime(
    anvil_dir *dir,
    int64_t *chunk_coords
) {
    if (anvil_assert(dir != nullptr) || anvil_assert(chunk_coords != nullptr)) return -1;

    anvil_result res = load_file(dir, chunk_coords, false);
    if (res != ANVIL_OK) {
        return -1;
    }
    if (!dir->file) {
        return 0;
    }
    return anvil_file_mtime(dir->file, chunk_coords);
}

uint8_t *anvil_read(
    anvil_dir *dir,
    size_t *n_bytes_read,
    const int64_t *chunk_coords
) {
    if (
        anvil_assert(dir != nullptr) ||
        anvil_assert(chunk_coords != nullptr)
    ) {
        return nullptr;
    }

    if (!dir) return nullptr;

    return nullptr;
}

bool anvil_write(
    anvil_dir *dir,
    const uint8_t *buf,
    const size_t n_bytes,
    const int64_t *chunk_coords
) {
    if (
        anvil_assert(dir != nullptr) ||
        anvil_assert(buf != nullptr || n_bytes > 0) ||
        anvil_assert(chunk_coords != nullptr)
    ) {
        return nullptr;
    }

    if (!dir) return -1;

    return false;
}