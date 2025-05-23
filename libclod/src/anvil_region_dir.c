/**
 * @private
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "anvil.h"
#include "anvil_internal.h"
#include "buffer.h"
#include "os.h"

#ifdef POSIX
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#elifdef WINDOWS
#error not implemented
#endif


#define JOIN(...) path_join(&region_dir->tmp_string, &region_dir->tmp_string_cap, region_dir->realloc, __VA_ARGS__)

anvil_result anvil_region_parse_name(
    const char *name,
    int64_t *region_x_out,
    int64_t *region_z_out
) {
    const size_t name_len = strlen(name);
    if (name_len == 0) {
        return ANVIL_INVALID_NAME;
    }

    // general idea is to start at the end of the string,
    // and keep moving down it until we find two consecutive '.'
    // that have a parsable integer following them.
    char *end_ptr;

    int i = (int)name_len - 2;
    while (i >= 0 && name[i] != '.') i--;
    if (i < 0) return ANVIL_INVALID_NAME;

next:
    const int j = i;
    i--;
    while (i >= 0 && name[i] != '.') i--;
    if (i < 0) return ANVIL_INVALID_NAME;

    errno = 0;
    *region_x_out = strtoll(name + i + 1, &end_ptr, 10);
    if (errno != 0 || end_ptr == name + i + 1) goto next;
    *region_z_out = strtoll(name + j + 1, &end_ptr, 10);
    if (errno != 0 || end_ptr == name + j + 1) goto next;

    return ANVIL_OK;
}

/**
 * @private
 *
 * *very* intentionally do *not* store the path to the region directory.
 * using the path opens us up to race conditions and other issues,
 * instead we should do what the OS encourages us to do and use directory file descriptors.
 */
struct anvil_region_dir {
    char *subdir;           /** path to the region directory relative to the world directory. */
    char *region_extension; /** file name extension that region files have. */
    char *chunk_extension;  /** file name extension that chunk files have. */

    char *tmp_string;       /** (nullable) temporary string. */
    size_t tmp_string_cap;  /** allocated size of the temporary string. */

    const anvil_allocator *alloc; /** custom allocator. */

#ifdef POSIX
    int dir_fd;
#else
#error not implemented
#endif
};

anvil_result anvil_region_dir_open(
    struct anvil_region_dir **region_dir_out,
    const char *path,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,
    const anvil_allocator *alloc
) {
    if (
        region_dir_out == nullptr ||
        path == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

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

#ifdef POSIX

    const int dir_fd = open(path, O_RDONLY | O_DIRECTORY);
    if (dir_fd < 0) {
        switch (errno) {
        case ENOENT: errno = 0; return ANVIL_NOT_EXIST;
        case ENOMEM: errno = 0; return ANVIL_ALLOC_FAILED;
        case ENOTDIR: errno = 0; return ANVIL_NOT_EXIST;
        default: return ANVIL_IO_ERROR;
        }
    }

    const anvil_result res = anvil_region_dir_openat(
        region_dir_out,
        subdir,
        region_extension,
        chunk_extension,
        dir_fd,
        alloc
    );

    if (res != ANVIL_OK) {
        const auto err = errno;
        close(dir_fd);
        errno = err;
        return res;
    }

    if (close(dir_fd)) {
        return ANVIL_IO_ERROR;
    }
    return ANVIL_OK;

#else
#error not implemented
#endif
}

anvil_result anvil_region_dir_openat(
    struct anvil_region_dir **region_dir_out,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,

#ifdef POSIX
    const int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc
) {
    if (region_dir_out == nullptr) {
        return ANVIL_INVALID_USAGE;
    }

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

    struct anvil_region_dir *region_dir = alloc->malloc(sizeof(struct anvil_region_dir));
    if (region_dir == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

    region_dir->subdir = string_copy(alloc->malloc, subdir, ".");
    region_dir->region_extension = string_copy(alloc->malloc, region_extension, "mca");
    region_dir->chunk_extension = string_copy(alloc->malloc, chunk_extension, "mcc");

    if (
        region_dir->subdir == nullptr ||
        region_dir->region_extension == nullptr ||
        region_dir->chunk_extension == nullptr
    ) {
        alloc->free(region_dir->subdir);
        alloc->free(region_dir->region_extension);
        alloc->free(region_dir->chunk_extension);
        alloc->free(region_dir);
        return ANVIL_ALLOC_FAILED;
    }

    region_dir->tmp_string = nullptr;
    region_dir->tmp_string_cap = 0;
    region_dir->alloc = alloc;

#ifdef POSIX

    region_dir->dir_fd = openat(dir_fd, region_dir->subdir, O_RDONLY | O_DIRECTORY);
    if (region_dir->dir_fd < 0) {
        alloc->free(region_dir->subdir);
        alloc->free(region_dir->region_extension);
        alloc->free(region_dir->chunk_extension);
        alloc->free(region_dir);

        switch (errno) {
        case ENOENT: errno = 0; return ANVIL_NOT_EXIST;
        case ENOMEM: errno = 0; return ANVIL_ALLOC_FAILED;
        case ENOTDIR: errno = 0; return ANVIL_NOT_EXIST;
        default: return ANVIL_IO_ERROR;
        }
    }

#else
#error not implemented
#endif

    *region_dir_out = region_dir;
    return ANVIL_OK;
}

struct anvil_region_iter {
    const struct anvil_region_dir *region_dir;

    int64_t region_x;
    int64_t region_z;

#ifdef POSIX

    DIR *dir;
    int ent_fd;

#else
#error not implemented
#endif
};

anvil_result anvil_region_iter_open(
    struct anvil_region_iter **region_iter_out,
    const struct anvil_region_dir *region_dir
) {
    if (
        region_iter_out == nullptr ||
        region_dir == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

    struct anvil_region_iter *region_iter = region_dir->alloc->malloc(sizeof(struct anvil_region_iter));
    if (region_iter == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

    region_iter->region_dir = region_dir;

#ifdef POSIX

    region_iter->ent_fd = -1;
    region_iter->dir = fdopendir(region_dir->dir_fd);
    if (region_iter->dir == nullptr) {
        region_dir->alloc->free(region_iter);
        switch (errno) {
        case ENOENT: return ANVIL_NOT_EXIST;
        case ENOMEM: return ANVIL_ALLOC_FAILED;
        default: return ANVIL_IO_ERROR;
        }
    }

#else
#error not implemented
#endif

    *region_iter_out = region_iter;
    return ANVIL_OK;
}

bool validate_region_filename(
    const struct anvil_region_dir *region_dir,
    const char *name,
    int64_t *region_x,
    int64_t *region_z
) {
    if (name == nullptr) return false;
    assert(region_x != nullptr);
    assert(region_z != nullptr);

    size_t name_len = strlen(name);
    size_t ext_len = strlen(region_dir->region_extension);

    while (name_len > 0 && ext_len > 0) {
        if (name[--name_len] != region_dir->region_extension[--ext_len]) {
            return false;
        }
    }
    if (name_len == 0) return false;

    if (anvil_region_parse_name(name, region_x, region_z) != ANVIL_OK) {
        return false;
    }

    return true;
}

anvil_result anvil_region_iter_next(
    struct anvil_region_entry *entry,
    struct anvil_region_iter *region_iter
) {
    if (
        entry == nullptr ||
        region_iter == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

#ifdef POSIX

    if (region_iter->ent_fd >= 0) {
        close(region_iter->ent_fd);
    }

next_file:
    const struct dirent *ent = readdir(region_iter->dir);
    if (ent == nullptr) return ANVIL_DONE;

    if (!validate_region_filename(region_iter->region_dir, ent->d_name, &region_iter->region_x, &region_iter->region_z))
        goto next_file;

    // so this is a bit of an odd place to open the region files,
    // and we may not even actually want to open the file.
    // the reason we do this here is because it gives us two things.
    //
    // Firstly, it's a second check for the existence of the file,
    // as well as a more thorough test for the file's validity.
    // If the region file was deleted in the meantime then we catch it here.
    // This is important because this iteration is probably going *very* slowly indeed.
    // In fact, the entire lifetime of the program is probably spent in this iteration.
    // File changes *need* to be appropriately handled - they are not errors, but an expected outcome.
    //
    // Secondly, if we do have a valid region file on our hands, chances are we *do* want to open it,
    // and holding onto a file descriptor is better than a filepath.
    // See the rationale for the openat method family for why.
    region_iter->ent_fd = openat(region_iter->region_dir->dir_fd, ent->d_name, O_RDWR);
    if (region_iter->ent_fd < 0) {
        switch (errno) {
        case EACCES: case ELOOP: case ENAMETOOLONG: case ENOENT: case EOVERFLOW:
            errno = 0; goto next_file;
        case ENOMEM:
            return ANVIL_ALLOC_FAILED;
        default:
            return ANVIL_IO_ERROR;
        }
    }

    struct stat st;
    if (fstat(region_iter->ent_fd, &st)) {
        switch (errno) {
        case EACCES: case ELOOP: case ENAMETOOLONG: case ENOENT: case EOVERFLOW:
            errno = 0; goto next_file;
        case ENOMEM:
            return ANVIL_ALLOC_FAILED;
        default:
            return ANVIL_IO_ERROR;
        }
    }

    entry->subdir = region_iter->region_dir->subdir;
    entry->filename = ent->d_name;
    entry->region_x = region_iter->region_x;
    entry->region_z = region_iter->region_z;
    entry->mtime = st.st_mtime;
    entry->size = st.st_size;

#else
#not implemented
#endif

    return ANVIL_OK;
}

anvil_result anvil_region_iter_open_file(
    struct anvil_region_file **region_file_out,
    const struct anvil_region_iter *region_iter
) {
    if (
        region_file_out == nullptr ||
        region_iter == nullptr
    ) return ANVIL_INVALID_USAGE;

    return anvil_region_file_new(
        region_file_out,
        region_iter->region_x,
        region_iter->region_z,
        region_iter->region_dir->chunk_extension,

#ifdef POSIX

        region_iter->region_dir->dir_fd,

#else
#error not implemented
#endif

        region_iter->region_dir->alloc
    );
}

void anvil_region_iter_close(
    struct anvil_region_iter *region_iter
) {
#ifdef POSIX
    if (region_iter->ent_fd >= 0)
        close(region_iter->ent_fd);

    closedir(region_iter->dir);
#else
#error not implemented
#endif

    region_iter->region_dir->alloc->free(region_iter);
}

anvil_result anvil_region_open_file(
    struct anvil_region_file **region_file_out,
    struct anvil_region_dir *region_dir,
    const int64_t region_x,
    const int64_t region_z
) {
    if (
        region_file_out == nullptr ||
        region_dir == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

    return anvil_region_file_new(
        region_file_out,
        region_x,
        region_z,
        region_dir->chunk_extension,
#ifdef POSIX
        region_dir->dir_fd,
#else
#error not implemented
#endif
        region_dir->alloc
    );

try_again:
    const int path_len = anvil_region_filename(
        region_dir->tmp_string,
        region_dir->tmp_string_cap,
        "r",
        region_x,
        region_z,
        region_dir->region_extension
    );

    if (path_len >= region_dir->tmp_string_cap) {
        char *new = region_dir->alloc->realloc(region_dir->tmp_string, path_len);
        if (new == nullptr) return ANVIL_ALLOC_FAILED;

        region_dir->tmp_string = new;
        region_dir->tmp_string_cap = path_len;
        goto try_again;
    }

    return anvil_region_file_open(
        region_file_out,
        region_dir->tmp_string,
        region_dir->chunk_extension,
        region_dir->alloc
    );
}

anvil_result anvil_region_remove(
    struct anvil_region_dir *region_dir,
    const int64_t region_x,
    const int64_t region_z
) {
try_region_name_again:
    int path_len = anvil_region_filename(
        region_dir->tmp_string,
        region_dir->tmp_string_cap,
        "r",
        region_x,
        region_z,
        region_dir->region_extension
    );

    if (path_len >= region_dir->tmp_string_cap) {
        char *new = region_dir->alloc->realloc(region_dir->tmp_string, path_len);
        if (new == nullptr) return ANVIL_ALLOC_FAILED;

        region_dir->tmp_string = new;
        region_dir->tmp_string_cap = path_len;
        goto try_region_name_again;
    }

#ifdef POSIX
    if (
        unlinkat(region_dir->dir_fd, region_dir->tmp_string, 0) != 0 &&
        errno != ENOENT
    ) {
        return ANVIL_IO_ERROR;
    }
#else
#error undefined
#endif

    // so this is a bit interesting.
    // say the region file is corrupt - what would a user expect this method to do?
    // probably its job - but we don't know if chunk data is split into dedicated files
    // unless we can successfully parse the region file - and the chunk headers are
    // spread out through the whole file no less!
    //
    // so instead of seeking and jumping through the entire region file,
    // reading one byte at a time and failing if the region file is corrupt,
    // we just try to delete all possible chunk files.
    //
    // I mean goodness gracious, if the region file is corrupted *this* is probably
    // the method one would use to try to resolve the problem.
    // we really ought to make sure this operation succeeds.

    for (int64_t chunk_x = 0; chunk_x < 32; chunk_x++) for (int64_t chunk_z = 0; chunk_z < 32; chunk_z++) {
    try_chunk_name_again:
        path_len = anvil_region_filename(
            region_dir->tmp_string,
            region_dir->tmp_string_cap,
            "r",
            region_x,
            region_z,
            region_dir->region_extension
        );

        if (path_len >= region_dir->tmp_string_cap) {
            char *new = region_dir->alloc->realloc(region_dir->tmp_string, path_len);
            if (new == nullptr) return ANVIL_ALLOC_FAILED;

            region_dir->tmp_string = new;
            region_dir->tmp_string_cap = path_len;
            goto try_chunk_name_again;
        }

#ifdef POSIX
        if (
            unlinkat(region_dir->dir_fd, region_dir->tmp_string, 0) != 0 &&
            errno != ENOENT
        ) {
            return ANVIL_IO_ERROR;
        }
#else
#error not implemented
#endif
    }

    return ANVIL_OK;
}

void anvil_region_dir_close(struct anvil_region_dir *region_dir) {
#ifdef POSIX
    close(region_dir->dir_fd);
#else
#error not implemented
#endif

    region_dir->alloc->free(region_dir->subdir);
    region_dir->alloc->free(region_dir->region_extension);
    region_dir->alloc->free(region_dir->chunk_extension);
    region_dir->alloc->free(region_dir->tmp_string);
    region_dir->alloc->free(region_dir);
}
