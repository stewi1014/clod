#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include "anvil.h"
#include "path.h"

/// @private
struct anvil_region_file {
    char *path;
    FILE *file; /** open file handle. set to null if closed after error. */

    /**
     * true if both the file and this struct do not contain a valid header.
     *
     * the header is written lazily to avoid needlessly filling
     * region files with zeroes if they are opened and no chunk data is written to them.
     * minecraft tends to leave empty region files around sometimes,
     * and not doing this would mean that us touching them fills them with zeroes.
     */
    bool needs_header;

    char *tmp_string;       /** (nullable) temporary string. */
    size_t tmp_string_cap;  /** allocated size of the temporary string. */

    char *chunk_extension; /** the filename extension that chunk files have. */

    int64_t region_x;
    int64_t region_z;
    const anvil_allocator *alloc;

    uint32_t chunk_offsets[32][32]; /** offset in 4096 byte sectors of the chunk's data. */
    uint32_t chunk_mtimes[32][32];  /** last modification in epoch seconds. */
    uint8_t chunk_sizes[32][32];    /** size in 4096 byte sectors of the chunk's data. */
};

anvil_result anvil_region_file_open(
    struct anvil_region_file **region_file_out,
    const char *path,
    const char *chunk_extension,
    const anvil_allocator *alloc
) {
    if (region_file_out == nullptr || path == nullptr)
        return ANVIL_INVALID_ARGUMENT;
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

    if (strlen(path) == 0) {
        return ANVIL_NOT_EXIST;
    }

    int64_t region_x, region_z;



    struct anvil_region_file *region_file = alloc->malloc(sizeof(struct anvil_region_file));
    if (region_file == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

    region_file->path = alloc->malloc(strlen(path) + 1);
    if (region_file->path == nullptr) {
        alloc->free(region_file);
        return ANVIL_ALLOC_FAILED;
    }
    strcpy(region_file->path, path);

    if (chunk_extension == nullptr) chunk_extension = "mcc";
    region_file->chunk_extension = alloc->malloc(strlen(chunk_extension) + 1);
    if (region_file->chunk_extension == nullptr) {
        alloc->free(region_file->path);
        alloc->free(region_file);
        return ANVIL_ALLOC_FAILED;
    }
    strcpy(region_file->chunk_extension, chunk_extension);

    region_file->tmp_string = nullptr;
    region_file->tmp_string_cap = 0;
    region_file->alloc = alloc;

    // need to open for reading and writing,
    // create the file if it exists,
    // and *not* truncate it if it doesn't already exist.
    // unfortunately it seems that fopen can't do that - so I use lower level methods to open the file.

    #ifdef _WIN32

    #error TODO: open file without truncating if exists else create

    #else

    const int fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->path);
        alloc->free(region_file);
        switch (errno) {
        case ENOENT: errno = 0; return ANVIL_NOT_EXIST;
        case ENOMEM: errno = 0; return ANVIL_ALLOC_FAILED;
        case ENOSPC: errno = 0; return ANVIL_DISK_FULL;
        case ENOTDIR: errno = 0; return ANVIL_NOT_EXIST;
        default: return ANVIL_IO_ERROR;
        }
    }

    region_file->file = fdopen(fd, "r+");
    if (region_file->file == nullptr) {
        // I don't think I really have much recourse if closing the fd here fails.
        // besides, it's already got to be a pretty extreme situation to get here in the first place.
        // so I hope that the first error is more helpful to the user, preserve it and discard errors form close.
        const auto err = errno;
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->path);
        alloc->free(region_file);
        close(fd);
        errno = err;
        return ANVIL_IO_ERROR;
    }

    #endif

    char *restrict buffer_view = region_file->chunk_offsets;

    const size_t r = fread(buffer_view, 4096, 2, region_file->file);
    if (r != 2) {
        if (feof(region_file->file)) {
            region_file->needs_header = true;
            return ANVIL_OK;
        }

        const auto err = errno;
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->path);
        alloc->free(region_file);
        fclose(region_file->file);
        region_file->file = nullptr;
        errno = err;
        return ANVIL_IO_ERROR;
    }

    for (int i = 0; i < 1024; i++) {
        const uint32_t offset =
            (unsigned char)buffer_view[i * 4 + 0] << (2 * 8) |
            (unsigned char)buffer_view[i * 4 + 1] << (1 * 8) |
            (unsigned char)buffer_view[i * 4 + 2] << (0 * 8) ;

        const uint8_t size =
            (unsigned char)buffer_view[i * 4 + 3];

        region_file->chunk_offsets[i/32][i%32] = offset;
        region_file->chunk_sizes[i/32][i%32] = size;
    }

    for (int i = 1024; i < 2048; i++) {
        const uint32_t mtime =
            (unsigned char)buffer_view[i * 4 + 0] << (3 * 8) |
            (unsigned char)buffer_view[i * 4 + 1] << (2 * 8) |
            (unsigned char)buffer_view[i * 4 + 2] << (1 * 8) |
            (unsigned char)buffer_view[i * 4 + 3] << (0 * 8) ;

        region_file->chunk_mtimes[i/32][i%32] = mtime;
    }

    region_file->needs_header = false;

    return ANVIL_OK;
}

char *anvil_chunk_file_path(
    struct anvil_region_file *region_file,
    const int64_t chunk_x,
    const int64_t chunk_z
) {
    try_again:
        const int name_len = snprintf(
            region_file->tmp_string,
            region_file->tmp_string_cap,
            "%s%sc.%ld.%ld.%s",
            region_file->path,
            PATH_SEP,
            chunk_x,
            chunk_z,
            region_file->chunk_extension
        );

    if (name_len >= region_file->tmp_string_cap) {
        char *new = region_file->alloc->realloc(region_file->tmp_string, name_len);
        if (new == nullptr) return nullptr;
        region_file->tmp_string = new;
        region_file->tmp_string_cap = name_len;
        goto try_again;
    }

    return region_file->tmp_string;
}

uint32_t anvil_chunk_mtime(
    const struct anvil_region_file *region_file,
    const int64_t chunk_x,
    const int64_t chunk_z
) {
    if (
        region_file == nullptr ||
        region_file->needs_header ||
        region_file->file == nullptr
    ) {
        return 0;
    }

    return region_file->chunk_mtimes[chunk_x&31][chunk_z&31];
}

anvil_result anvil_chunk_read(
    void *restrict out,
    size_t out_cap,
    size_t *out_len,
    const int64_t chunk_x,
    const int64_t chunk_z,
    struct anvil_region_file *region_file
) {
    if (
        region_file == nullptr ||
        out_len == nullptr ||
        region_file->file == nullptr
    ) {
        return ANVIL_INVALID_ARGUMENT;
    }

    const uint32_t offset = region_file->chunk_offsets[chunk_x&31][chunk_z&31];
    uint8_t size = region_file->chunk_sizes[chunk_x&31][chunk_z&31];

    if (fseek(region_file->file, offset * 4096, SEEK_SET) == -1)
        return ANVIL_IO_ERROR;

    uint8_t header[5];
    if (fread(header, 1, 5, region_file->file) != 5) {
        if (feof(region_file->file)) {
            fclose(region_file->file);
            region_file->file = nullptr;
            return ANVIL_MALFORMED;
        }

        const auto err = errno;
        fclose(region_file->file);
        errno = err;
        region_file->file = nullptr;
        return ANVIL_IO_ERROR;
    }

    FILE *file = region_file->file;
    if (header[4] & 128) {
        // chunk data is stored in a separate file.

        char *chunk_file_path = anvil_chunk_file_path()

        file = fopen()
    }
}

anvil_result anvil_chunk_write(
    void *restrict in,
    size_t in_len,
    enum anvil_compression compression,
    int64_t chunk_x,
    int64_t chunk_z,
    struct anvil_region_file *region_file
) {
    if (
        region_file == nullptr ||
        (in_len > 0 && in == nullptr)
    ) {
        return ANVIL_INVALID_ARGUMENT;
    }
}

void anvil_region_file_close(struct anvil_region_file *region_file) {
    if (region_file->file != nullptr) {
        fclose(region_file->file);
        region_file->file = nullptr;
    }

    region_file->alloc->free(region_file);
}