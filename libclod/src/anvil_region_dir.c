/**
 * @private
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "anvil.h"
#include "path.h"

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

    int i = name_len - 2;
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

/// @private
struct anvil_region_dir {
    char *path;             /** path to the region directory. */
    char *subdir;           /** (nullable) path to the region directory relative to the world directory. */
    char *region_extension; /** file name extension that region files have. */
    char *chunk_extension;  /** file name extension that chunk files have. */

    char *tmp_string;       /** (nullable) temporary string. */
    size_t tmp_string_cap;  /** allocated size of the temporary string. */

    const anvil_allocator *alloc; /** custom allocator. */
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
        return ANVIL_INVALID_ARGUMENT;
    }

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

    const size_t path_len = strlen(path);
    if (path_len == 0) {
        return ANVIL_NOT_EXIST;
    }

    if (region_extension == nullptr) region_extension = "mca";
    if (chunk_extension == nullptr) chunk_extension = "mcc";

    char *path_copy = alloc->malloc(path_len + 1);
    if (path_copy == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }
    strcpy(path_copy, path);

    char *subdir_copy = nullptr;
    if (subdir != nullptr) {
        subdir_copy = alloc->malloc(strlen(subdir) + 1);
        if (subdir_copy == nullptr) {
            alloc->free(path_copy);
            return ANVIL_ALLOC_FAILED;
        }
    }

    char *region_extension_copy = alloc->malloc(strlen(region_extension) + 1);
    if (region_extension_copy == nullptr) {
        if (subdir_copy != nullptr) alloc->free(subdir_copy);
        alloc->free(path_copy);
        return ANVIL_ALLOC_FAILED;
    }
    strcpy(region_extension_copy, region_extension);

    char *chunk_extension_copy = alloc->malloc(strlen(chunk_extension) + 1);
    if (chunk_extension_copy == nullptr) {
        alloc->free(region_extension_copy);
        if (subdir_copy != nullptr) alloc->free(subdir_copy);
        alloc->free(path_copy);
        return ANVIL_ALLOC_FAILED;
    }
    strcpy(chunk_extension_copy, chunk_extension);

    struct anvil_region_dir *region_dir = alloc->malloc(sizeof(struct anvil_region_dir));
    if (region_dir == nullptr) {
        alloc->free(chunk_extension_copy);
        alloc->free(region_extension_copy);
        if (subdir_copy != nullptr) alloc->free(subdir_copy);
        alloc->free(path_copy);
        return ANVIL_ALLOC_FAILED;
    }

    region_dir->path = path_copy;
    region_dir->subdir = subdir_copy;
    region_dir->region_extension = region_extension_copy;
    region_dir->chunk_extension = chunk_extension_copy;
    region_dir->tmp_string = nullptr;
    region_dir->tmp_string_cap = 0;
    region_dir->alloc = alloc;

    *region_dir_out = region_dir;
    return ANVIL_OK;
}

struct anvil_region_iter {
    DIR *dir;
    char *path;
    char *subdir;
    const anvil_allocator *alloc;

    char *region_extension;
    size_t region_extension_len;

    char *tmp_string;
    size_t tmp_string_cap;
};

anvil_result anvil_region_iter_open(
    struct anvil_region_iter **region_iter_out,
    const struct anvil_region_dir *region_dir
) {
    if (
        region_iter_out == nullptr ||
        region_dir == nullptr
    ) {
        return ANVIL_INVALID_ARGUMENT;
    }

    struct anvil_region_iter *region_iter = region_dir->alloc->malloc(sizeof(struct anvil_region_iter));
    if (region_iter == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

    region_iter->dir = opendir(region_dir->path);
    if (region_iter->dir == nullptr) {
        region_dir->alloc->free(region_iter);
        switch (errno) {
        case ENOENT: return ANVIL_NOT_EXIST;
        case ENOMEM: return ANVIL_ALLOC_FAILED;
        default: return ANVIL_IO_ERROR;
        }
    }

    region_iter->alloc = region_dir->alloc;
    region_iter->path = region_dir->path;
    region_iter->subdir = region_dir->subdir;
    region_iter->region_extension = region_dir->region_extension;
    region_iter->region_extension_len = strlen(region_dir->region_extension);

    region_iter->tmp_string = nullptr;
    region_iter->tmp_string_cap = 0;

    *region_iter_out = region_iter;
    return ANVIL_OK;
}

anvil_result anvil_region_iter_next(
    struct anvil_region_entry *entry,
    struct anvil_region_iter *region_iter
) {
    if (
        entry == nullptr ||
        region_iter == nullptr
    ) {
        return ANVIL_INVALID_ARGUMENT;
    }

next_file:
    const struct dirent* ent = readdir(region_iter->dir);
    if (ent == nullptr) {
        return ANVIL_DONE;
    }

    const size_t name_len = strlen(ent->d_name);
    if (name_len < region_iter->region_extension_len) {
        goto next_file;
    }

    for (size_t i = 1; i < region_iter->region_extension_len + 1; i++) {
        if (ent->d_name[name_len - i] != region_iter->region_extension[region_iter->region_extension_len - i]) {
            goto next_file;
        }
    }

    int64_t region_x, region_z;
    if (anvil_region_parse_name(ent->d_name, &region_x, &region_z) != ANVIL_OK) {
        goto next_file;
    }

    char *path = path_join(
        &region_iter->tmp_string, &region_iter->tmp_string_cap, region_iter->alloc,
        region_iter->path, PATH_SEP, ent->d_name, nullptr
    );

    struct stat st;
    if (stat(path,&st)) {
        switch (errno) {
        case (EACCES): errno = 0; goto next_file;
        case (ELOOP): errno = 0; goto next_file;
        case (ENAMETOOLONG): errno = 0; goto next_file;
        case (ENOENT): errno = 0; goto next_file;
        case (EOVERFLOW): errno = 0; goto next_file;
        case (ENOMEM): return ANVIL_ALLOC_FAILED;
        default: return ANVIL_IO_ERROR;
        }
    }

    entry->path = path;
    entry->subdir = region_iter->subdir;
    entry->filename = ent->d_name;
    entry->region_x = region_x;
    entry->region_z = region_z;
    entry->mtime = st.st_mtime;
    entry->size = st.st_size;
    entry->blksize = st.st_blksize;
    entry->blkcnt = st.st_blocks;

    return ANVIL_OK;
}

void anvil_region_iter_close(
    struct anvil_region_iter *region_iter
) {
    if (region_iter->tmp_string != nullptr)
        region_iter->alloc->free(region_iter->tmp_string);

    region_iter->alloc->free(region_iter);
}

char *anvil_region_file_path(
    struct anvil_region_dir *region_dir,
    const int64_t region_x,
    const int64_t region_z
) {
try_again:
    const int name_len = snprintf(
        region_dir->tmp_string,
        region_dir->tmp_string_cap,
        "%s%sr.%ld.%ld.%s",
        region_dir->path,
        PATH_SEP,
        region_x,
        region_z,
        region_dir->region_extension
    );

    if (name_len >= region_dir->tmp_string_cap) {
        char *new = region_dir->alloc->realloc(region_dir->tmp_string, name_len);
        if (new == nullptr) return nullptr;
        region_dir->tmp_string = new;
        region_dir->tmp_string_cap = name_len;
        goto try_again;
    }

    return region_dir->tmp_string;
}

anvil_result anvil_region_open_file(
    struct anvil_region_file **region_file_out,
    struct anvil_region_dir *region_dir,
    const int64_t region_x,
    const int64_t region_z
) {
    if (region_file_out == nullptr) return ANVIL_INVALID_ARGUMENT;

    const char *name = anvil_region_file_path(region_dir, region_x, region_z);
    if (name == nullptr) return ANVIL_ALLOC_FAILED;

    return anvil_region_file_open(
        region_file_out,
        name,
        region_dir->chunk_extension,
        region_dir->alloc
    );
}

anvil_result anvil_region_remove(
    struct anvil_region_dir *region_dir,
    const int64_t region_x,
    const int64_t region_z
) {
    const char *region_file_path = anvil_region_file_path(region_dir, region_x, region_z);
    if (region_file_path == nullptr) return ANVIL_ALLOC_FAILED;

    int ret = remove(region_file_path);
    if (ret != 0 && errno != ENOENT) {
        return ANVIL_IO_ERROR;
    }

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
    // we really ought to make sure this operation can succeed.

    for (int64_t chunk_x = 0; chunk_x < 32; chunk_x++)
    for (int64_t chunk_z = 0; chunk_z < 32; chunk_z++) {


        char *chunk_file_path = anvil_chunk_file_path(
            region_dir, region_x * 32 + chunk_x, region_z * 32 + chunk_z
        );

        ret = remove(chunk_file_path);
        if (ret != 0 && errno != ENOENT) {
            if (anvil_messages != nullptr) {
                fprintf(anvil_messages, "deleting chunk file: %s (%s)\n", strerror(errno), chunk_file_path);
            }
            return ANVIL_IO_ERROR;
        }
    }

    return ANVIL_OK;
}

void anvil_region_dir_close(struct anvil_region_dir *region_dir) {
    region_dir->alloc->free(region_dir->path);
    if (region_dir->subdir != nullptr)
        region_dir->alloc->free(region_dir->subdir);
    region_dir->alloc->free(region_dir->region_extension);
    region_dir->alloc->free(region_dir->chunk_extension);
    if (region_dir->tmp_string != nullptr)
        region_dir->alloc->free(region_dir->tmp_string);
    region_dir->alloc->free(region_dir);
}
