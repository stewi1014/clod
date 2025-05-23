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

#ifdef CLOD_USE_POSIX

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#else
#error not implemented
#endif

/**
 * @private
 *
 * *very* intentionally do *not* store the path to the region directory.
 * using the path opens us up to race conditions and other issues,
 * instead we should do what the OS encourages us to do and use directory file descriptors.
 */
struct anvil_dir {
    char *subdir;           /** path to the region directory relative to the world directory. */
    char *region_extension; /** file name extension that region files have. */
    char *chunk_extension;  /** file name extension that chunk files have. */

    char *tmp_string;       /** (nullable) temporary string. */
    size_t tmp_string_cap;  /** allocated size of the temporary string. */

    const anvil_allocator *alloc; /** custom allocator. */

#ifdef CLOD_USE_POSIX
    int dir_fd;
#else
#error not implemented
#endif
};

anvil_result anvil_dir_open(
    struct anvil_dir **dir_out,
    const char *path,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,
    const anvil_allocator *alloc
) {
    __anvil_assert_return(dir_out != nullptr);
    __anvil_assert_return(path != nullptr);

    if (alloc == nullptr)
        alloc = &default_anvil_allocator;

    __anvil_assert_return(alloc->malloc != nullptr);
    __anvil_assert_return(alloc->calloc != nullptr);
    __anvil_assert_return(alloc->free != nullptr);
    __anvil_assert_return(alloc->realloc != nullptr);

#ifdef CLOD_USE_POSIX

    const int dir_fd = open(path, O_RDONLY | O_DIRECTORY);
    if (dir_fd < 0) {
        return __anvil_errno;
    }

    const anvil_result res = anvil_region_dir_openat(
        dir_out,
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
        return __anvil_errno;
    }
    return ANVIL_OK;

#else
#error not implemented
#endif
}

anvil_result anvil_region_dir_openat(
    struct anvil_dir **region_dir_out,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,

#ifdef CLOD_USE_POSIX
    const int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc
) {
    __anvil_assert_return(region_dir_out != nullptr);

    if (alloc == nullptr)
        alloc = &default_anvil_allocator;

    __anvil_assert_return(alloc->malloc != nullptr);
    __anvil_assert_return(alloc->calloc != nullptr);
    __anvil_assert_return(alloc->free != nullptr);
    __anvil_assert_return(alloc->realloc != nullptr);

    struct anvil_dir *region_dir = alloc->malloc(sizeof(struct anvil_dir));
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

#ifdef CLOD_USE_POSIX

    region_dir->dir_fd = openat(dir_fd, region_dir->subdir, O_RDONLY | O_DIRECTORY);
    if (region_dir->dir_fd < 0) {
        alloc->free(region_dir->subdir);
        alloc->free(region_dir->region_extension);
        alloc->free(region_dir->chunk_extension);
        alloc->free(region_dir);
        return __anvil_errno;
    }

#else
#error not implemented
#endif

    *region_dir_out = region_dir;
    return ANVIL_OK;
}

struct anvil_iter {
    const struct anvil_dir *region_dir;

    int64_t region_x;
    int64_t region_z;

#ifdef CLOD_USE_POSIX

    DIR *dir;
    int ent_fd;

#else
#error not implemented
#endif
};

anvil_result anvil_iter_open(
    struct anvil_iter **iter_out,
    const struct anvil_dir *dir
) {
    if (
        iter_out == nullptr ||
        dir == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

    struct anvil_iter *region_iter = dir->alloc->malloc(sizeof(struct anvil_iter));
    if (region_iter == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

    region_iter->region_dir = dir;

#ifdef CLOD_USE_POSIX

    region_iter->ent_fd = -1;
    region_iter->dir = fdopendir(dir->dir_fd);
    if (region_iter->dir == nullptr) {
        dir->alloc->free(region_iter);
        return __anvil_errno;
    }

#else
#error not implemented
#endif

    *iter_out = region_iter;
    return ANVIL_OK;
}

bool validate_region_filename(
    const struct anvil_dir *region_dir,
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

    if (anvil_parse_name(name, region_x, region_z, nullptr) != ANVIL_OK) {
        return false;
    }

    return true;
}

anvil_result anvil_iter_next(
    struct anvil_entry *entry,
    struct anvil_iter *iter
) {
    if (
        entry == nullptr ||
        iter == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

#ifdef CLOD_USE_POSIX

    if (iter->ent_fd >= 0) {
        if (close(iter->ent_fd)) {
            return __anvil_errno;
        }
    }

next_file:
    const struct dirent *ent = readdir(iter->dir);
    if (ent == nullptr) return ANVIL_DONE;

    if (!validate_region_filename(iter->region_dir, ent->d_name, &iter->region_x, &iter->region_z))
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
    iter->ent_fd = openat(iter->region_dir->dir_fd, ent->d_name, O_RDWR);
    if (iter->ent_fd < 0) {
        if (
            errno == EACCES ||
            errno == ELOOP ||
            errno == ENAMETOOLONG ||
            errno == ENOENT ||
            errno == EISDIR ||
            errno == EINVAL ||
            errno == EOVERFLOW ||
            errno == EFBIG
        ) {
            errno = 0;
            goto next_file;
        }

        return __anvil_errno;
    }

    struct stat st;
    if (fstat(iter->ent_fd, &st)) {
        if (
            errno == EACCES ||
            errno == ELOOP ||
            errno == ENAMETOOLONG ||
            errno == ENOENT ||
            errno == EISDIR ||
            errno == EINVAL ||
            errno == EOVERFLOW ||
            errno == EFBIG
        ) {
            errno = 0;
            goto next_file;
        }

        return __anvil_errno;
    }

    entry->subdir = iter->region_dir->subdir;
    entry->filename = ent->d_name;
    entry->region_x = iter->region_x;
    entry->region_z = iter->region_z;
    entry->mtime = st.st_mtime;
    entry->size = st.st_size;

#else
#not implemented
#endif

    return ANVIL_OK;
}

anvil_result anvil_iter_open_file(
    struct anvil_file **file_out,
    const struct anvil_iter *iter
) {
    if (
        file_out == nullptr ||
        iter == nullptr
    ) return ANVIL_INVALID_USAGE;

    return anvil_region_file_openat(
        file_out,
        iter->region_x,
        iter->region_z,
        iter->region_dir->chunk_extension,
        iter->region_dir->region_extension,

#ifdef CLOD_USE_POSIX

        iter->region_dir->dir_fd,

#else
#error not implemented
#endif

        iter->region_dir->alloc
    );
}

void anvil_iter_close(
    struct anvil_iter *iter
) {
#ifdef CLOD_USE_POSIX
    if (iter->ent_fd >= 0)
        close(iter->ent_fd);

    closedir(iter->dir);
#else
#error not implemented
#endif

    iter->region_dir->alloc->free(iter);
}

anvil_result anvil_open_file(
    struct anvil_file **file_out,
    const struct anvil_dir *dir,
    const int64_t region_x,
    const int64_t region_z
) {
    if (
        file_out == nullptr ||
        dir == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

    return anvil_region_file_openat(
        file_out,
        region_x,
        region_z,
        dir->chunk_extension,
        dir->region_extension,

#ifdef CLOD_USE_POSIX

        dir->dir_fd,

#else
#error not implemented
#endif

        dir->alloc
    );
}

anvil_result anvil_remove(
    struct anvil_dir *dir,
    const int64_t region_x,
    const int64_t region_z
) {
try_region_name_again:
    int path_len = anvil_get_name(
        dir->tmp_string,
        dir->tmp_string_cap,
        "r",
        region_x,
        region_z,
        dir->region_extension
    );

    if (path_len >= dir->tmp_string_cap) {
        char *new = dir->alloc->realloc(dir->tmp_string, path_len);
        if (new == nullptr) return ANVIL_ALLOC_FAILED;

        dir->tmp_string = new;
        dir->tmp_string_cap = path_len;
        goto try_region_name_again;
    }

#ifdef CLOD_USE_POSIX

    if (
        unlinkat(dir->dir_fd, dir->tmp_string, 0) != 0 &&
        errno != ENOENT
    ) {
        return __anvil_errno;
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
        path_len = anvil_get_name(
            dir->tmp_string,
            dir->tmp_string_cap,
            "c",
            region_x * 32 + chunk_x,
            region_z * 32 + chunk_z,
            dir->chunk_extension
        );

        if (path_len >= dir->tmp_string_cap) {
            char *new = dir->alloc->realloc(dir->tmp_string, path_len);
            if (new == nullptr) return ANVIL_ALLOC_FAILED;

            dir->tmp_string = new;
            dir->tmp_string_cap = path_len;
            goto try_chunk_name_again;
        }

#ifdef CLOD_USE_POSIX

        if (
            unlinkat(dir->dir_fd, dir->tmp_string, 0) != 0 &&
            errno != ENOENT
        ) {
            return __anvil_errno;
        }

#else
#error not implemented
#endif
    }

    return ANVIL_OK;
}

void anvil_dir_close(struct anvil_dir *dir) {
#ifdef CLOD_USE_POSIX

    close(dir->dir_fd);

#else
#error not implemented
#endif

    dir->alloc->free(dir->subdir);
    dir->alloc->free(dir->region_extension);
    dir->alloc->free(dir->chunk_extension);
    dir->alloc->free(dir->tmp_string);
    dir->alloc->free(dir);
}
