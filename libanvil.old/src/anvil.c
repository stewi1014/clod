#include "anvil.h"

#include <stdio.h>

#include "vec.h"
#include "error.h"
#include "region.h"

#if HAVE_UNIX
#include <fcntl.h>
#include <unistd.h>
#elif HAVE_WINDOWS
#include <windows.h>
#else
#error not supported
#endif

/**
 * Open an anvil directory.
 * @param path Path to the directory containing region files.
 * @param opts Configuration options.
 * @return Handle to the directory.
 */
anvil *anvil_open(const char *path, const anvil_opts *opts) {
    if (!opts) {
        static anvil_opts default_opts;
        anvil_opts_default(&default_opts);
        opts = &default_opts;
    }

    if (
        error_assert(path != nullptr) ||
        error_assert(opts->region_extension != nullptr) ||
        error_assert(opts->chunk_extension != nullptr) ||
        error_assert(opts->coord_count > 0) ||
        error_assert(opts->coord_count <= ANVIL_MAX_COORDINATES) ||
        error_assert(opts->region_extent > 0) ||
        error_assert(opts->section_size > 0) ||
        error_assert(pow64(opts->region_extent, opts->coord_count) <= ANVIL_MAX_CHUNKS) ||
        error_assert(opts->malloc != nullptr) ||
        error_assert(opts->calloc != nullptr) ||
        error_assert(opts->free != nullptr) ||
        error_assert(opts->realloc != nullptr)
    ) {
        return nullptr;
    }

    anvil *a = opts->malloc(sizeof_anvil(path));
    if (!a) {
        error_result(ANVIL_ALLOC_FAILED, nullptr);
        return nullptr;
    }

    a->opts = *opts;
    mutex_init(&a->mtx);
    map_init(a->regions, (size_t) a->opts.coord_count * sizeof(int64_t), sizeof(region*), nullptr, nullptr, nullptr);
    strcpy(a->path, path);

    #if HAVE_UNIX

    a->dir_fd = open(a->path, O_RDONLY | O_DIRECTORY);
    if (a->dir_fd < 0) {
        error_errno("Opening region directory %s", a->path);
        a->opts.free(a);
        return nullptr;
    }

    #else
    #error not implemented
    #endif

    return a;
}
