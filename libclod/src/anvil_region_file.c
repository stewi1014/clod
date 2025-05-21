#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <libdeflate.h>

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
    bool is_empty;

    char *tmp_string;       /** (nullable) temporary string 2^8. */
    size_t tmp_string_cap;  /** allocated size of the temporary string. */

    char *tmp_buffer;       /** temporary buffer 2^16. */
    size_t tmp_buffer_cap;  /** allocated size of the temporary buffer. */

    bool read_continue;
    int64_t rc_chunk_x;
    int64_t rc_chunk_z;
    size_t rc_size;

    struct libdeflate_compressor *libdeflate_compressor;
    struct libdeflate_decompressor *libdeflate_decompressor;

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
        alloc->free == nullptr   ||
        alloc->realloc == nullptr
    ) {
        return ANVIL_INVALID_ARGUMENT;
    }

    if (strlen(path) == 0) {
        return ANVIL_NOT_EXIST;
    }

    int64_t region_x, region_z;
    const anvil_result result = anvil_region_parse_name(path, &region_x, &region_z);
    if (result != ANVIL_OK) return result;

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
    region_file->tmp_buffer = nullptr;
    region_file->tmp_buffer_cap = 0;
    region_file->read_continue = false;
    region_file->libdeflate_compressor = nullptr;
    region_file->libdeflate_decompressor = nullptr;
    region_file->region_x = region_x;
    region_file->region_z = region_z;
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
            region_file->is_empty = true;
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

    region_file->is_empty = false;
    return ANVIL_OK;
}

uint32_t anvil_chunk_mtime(
    const struct anvil_region_file *region_file,
    const int64_t chunk_x,
    const int64_t chunk_z
) {
    if (
        region_file == nullptr ||
        region_file->is_empty ||
        region_file->file == nullptr
    ) {
        return 0;
    }

    return region_file->chunk_mtimes[chunk_x&31][chunk_z&31];
}

anvil_result anvil_chunk_read(
    void *restrict out,
    const size_t out_cap,
    size_t *out_len,
    const int64_t chunk_x,
    const int64_t chunk_z,
    struct anvil_region_file *region_file
) {
    if (
        region_file == nullptr ||
        region_file->file == nullptr
    ) {
        return ANVIL_INVALID_ARGUMENT;
    }

    size_t size;

    if (
        region_file->read_continue &&
        region_file->rc_chunk_x == chunk_x &&
        region_file->rc_chunk_z == chunk_z
    ) {
        size = region_file->rc_size;
        goto continue_read;
    }

    if (region_file->is_empty) {
        *out_len = 0;
        return ANVIL_OK;
    }

    const auto offset = region_file->chunk_offsets[chunk_x&31][chunk_z&31];
    const auto sectors = region_file->chunk_sizes[chunk_x&31][chunk_z&31];

    if (offset < 2) {
        fclose(region_file->file);
        region_file->file = nullptr;
        return ANVIL_MALFORMED;
    }

    if (fseek(region_file->file, offset * 4096, SEEK_SET) == -1) {
        return ANVIL_IO_ERROR;
    }

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

    const uint32_t header_chunk_size =
        header[0] << 24 |
        header[1] << 16 |
        header[2] << 8  |
        header[3] - 1   ;
    const uint8_t header_compression_type =
        header[4] & 0b01111111;
    const bool header_chunk_file =
        header[4] & 0b10000000;

    FILE *file;
    bool close_on_exit;

    if (header_chunk_file) { // chunk data is stored in a separate file.
    try_chunk_name_again:
        const int path_len = anvil_filepath(
            region_file->tmp_string,
            region_file->tmp_string_cap,
            region_file->path,
            "c",
            region_file->region_x * 32 + (chunk_x & 31),
            region_file->region_z * 32 + (chunk_z & 31),
            region_file->chunk_extension
        );

        if (path_len >= region_file->tmp_string_cap) {
            char *new = region_file->alloc->realloc(region_file->tmp_string, path_len);
            if (new == nullptr) return ANVIL_ALLOC_FAILED;

            region_file->tmp_string = new;
            region_file->tmp_string_cap = path_len;
            goto try_chunk_name_again;
        }

        FILE *chunk_file = fopen(region_file->tmp_string, "rb");
        if (chunk_file == nullptr) {
        switch (errno) {
        case ENOENT:
            errno = 0;
            if (out_len != nullptr) *out_len = 0;
            return ANVIL_OK;
        case ENOMEM: errno = 0; return ANVIL_ALLOC_FAILED;
        case ENOTDIR: errno = 0; return ANVIL_NOT_EXIST;
        default: return ANVIL_IO_ERROR;
        }
        }

        if (fseek(chunk_file, 0, SEEK_END)) {
            const auto err = errno;
            fclose(chunk_file);
            errno = err;
            return ANVIL_IO_ERROR;
        }

        const auto file_size = ftell(chunk_file);
        if (file_size < 0) {
            const auto err = errno;
            fclose(chunk_file);
            errno = err;
            return ANVIL_IO_ERROR;
        }

        if (file_size == 0) {
            if (fclose(chunk_file)) {
                return ANVIL_IO_ERROR;
            }
            *out_len = 0;
            return ANVIL_OK;
        }

        size = file_size;
        close_on_exit = true;
        file = chunk_file;
    } else {
        if (header_chunk_size > sectors * 4096) {
            if (fclose(region_file->file)) {
                return ANVIL_IO_ERROR;
            }
            return ANVIL_MALFORMED;
        }

        size = header_chunk_size;
        close_on_exit = false;
        file = region_file->file;
    }

    switch (header_compression_type) {
    case ANVIL_COMPRESSION_NONE: {
        if (out_cap < size) {
            // it seems like a shame, but I doubt reopening this file
            // actually has a significant performance impact.
            if (close_on_exit && fclose(file)) {
                return ANVIL_IO_ERROR;
            }

            *out_len = size;
            return ANVIL_INSUFFICIENT_SPACE;
        }

        const auto n_read = fread(out, size, 1, file);
        if (n_read != 1) {
            if (feof(file)) {
                if (close_on_exit && fclose(file)) {
                    return ANVIL_IO_ERROR;
                }

                return ANVIL_MALFORMED;
            }

            if (close_on_exit) {
                const auto err = errno;
                fclose(file);
                errno = err;
            }
            return ANVIL_IO_ERROR;
        }

        *out_len = size;
        return ANVIL_OK;
    }

    case ANVIL_COMPRESSION_GZIP: case ANVIL_COMPRESSION_ZLIB: {
        if (region_file->tmp_buffer_cap < size) {
            char *new = region_file->alloc->realloc(region_file->tmp_buffer, size);
            if (new == nullptr) {
                if (close_on_exit) {
                    const auto err = errno;
                    fclose(file);
                    errno = err;
                }
                return ANVIL_ALLOC_FAILED;
            }

            region_file->tmp_buffer = new;
            region_file->tmp_buffer_cap = size;
        }

        const auto n_read = fread(region_file->tmp_buffer, size, 1, file);
        if (n_read != 1) {
            if (feof(file)) {
                if (close_on_exit && fclose(file)) {
                    return ANVIL_IO_ERROR;
                }

                return ANVIL_MALFORMED;
            }

            if (close_on_exit) {
                const auto err = errno;
                fclose(file);
                errno = err;
            }
            return ANVIL_IO_ERROR;
        }

        if (close_on_exit && fclose(file)) {
            return ANVIL_IO_ERROR;
        }

        if (region_file->libdeflate_decompressor == nullptr) {
            struct libdeflate_options opts = {0};
            opts.sizeof_options = sizeof(opts);
            opts.malloc_func = region_file->alloc->malloc;
            opts.free_func = region_file->alloc->free;
            region_file->libdeflate_decompressor = libdeflate_alloc_decompressor_ex(&opts);
        }

    continue_read:
        enum libdeflate_result res;
        if (header_compression_type == ANVIL_COMPRESSION_GZIP) {
            res = libdeflate_gzip_decompress_ex(
                region_file->libdeflate_decompressor,
                region_file->tmp_buffer,
                size,
                out,
                out_cap,
                nullptr,
                out_len
            );
        } else {
            res = libdeflate_gzip_decompress_ex(
                region_file->libdeflate_decompressor,
                region_file->tmp_buffer,
                size,
                out,
                out_cap,
                nullptr,
                out_len
            );
        }

        if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
            auto guess = (out_cap << 1) - (out_cap >> 1);
            if (guess == 0) guess = 1<<16;
            if (guess < size * 10) guess = size * 10;


            *out_len = size * 10;
            region_file->read_continue = true;
            region_file->rc_chunk_x = chunk_x;
            region_file->rc_chunk_z = chunk_z;
            return ANVIL_INSUFFICIENT_SPACE;
        }
        region_file->read_continue = false;
    }

    default: {
        if (close_on_exit && fclose(file)) {
            return ANVIL_IO_ERROR;
        }
        return ANVIL_UNSUPPORTED_COMPRESSION;
    }
    }
}

anvil_result anvil_chunk_write(
    const void *restrict in,
    const size_t in_len,
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
    return ANVIL_NOT_EXIST;
}

#define clear(var, op) if (var != nullptr) { op(var); var = nullptr; }

void anvil_region_file_close(struct anvil_region_file *region_file) {
    clear(region_file->path, region_file->alloc->free);
    clear(region_file->file, fclose);
    clear(region_file->tmp_string, region_file->alloc->free);
    region_file->tmp_string_cap = 0;
    clear(region_file->tmp_buffer, region_file->alloc->free);
    region_file->tmp_buffer_cap = 0;
    clear(region_file->libdeflate_decompressor, libdeflate_free_decompressor);
    clear(region_file->libdeflate_compressor, libdeflate_free_compressor);
    clear(region_file->chunk_extension, region_file->alloc->free);

    region_file->alloc->free(region_file);
}
