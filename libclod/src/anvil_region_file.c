#include <stdint.h>
#include <assert.h>
#include <libdeflate.h>
#include <math.h>
#include <stdlib.h>

#include "anvil.h"
#include "os.h"

#ifdef POSIX
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#elifdef WINDOWS
#error "windows methods not implemented yet"
#endif

#define SIZE_X 32
#define SIZE_Z 32

#define SECTOR_SIZE 4096
#define MTIME_OFFSET (SIZE_X * SIZE_Z * 4)
#define HEADER_SIZE (2 * SIZE_X * SIZE_Z * 4)

#define mod(a, b)\
    ((((a) % (b)) + (b)) % (b))

#define chunk_index(chunk_x, chunk_z)\
    ((mod((chunk_x), SIZE_X) * SIZE_Z + mod((chunk_z), SIZE_Z)) * 4)

#define get_chunk_sector_offset(data, index)\
    (uint32_t)(unsigned char)((data) + (index))[0] << (2 * 8) |\
    (uint32_t)(unsigned char)((data) + (index))[1] << (1 * 8) |\
    (uint32_t)(unsigned char)((data) + (index))[2] << (0 * 8) ;\

#define get_chunk_sector_count(data, index)\
    (uint8_t)(unsigned char)((data) + (index))[3];

#define get_chunk_mtime(data, index)\
    (uint32_t)(unsigned char)((data) + (index) + SECTOR_SIZE)[0] << (3 * 8) |\
    (uint32_t)(unsigned char)((data) + (index) + SECTOR_SIZE)[1] << (2 * 8) |\
    (uint32_t)(unsigned char)((data) + (index) + SECTOR_SIZE)[2] << (1 * 8) |\
    (uint32_t)(unsigned char)((data) + (index) + SECTOR_SIZE)[3] << (0 * 8) ;\

#define set_sector_offset(data, index, offset)\
    assert((offset) <= (1ULL<<24) - 1);\
    ((data) + (index))[0] = (uint32_t)(offset) >> (2 * 8);\
    ((data) + (index))[1] = (uint32_t)(offset) >> (1 * 8);\
    ((data) + (index))[2] = (uint32_t)(offset) >> (0 * 8);

#define set_sector_count(data, index, count) \
    assert((count) <= (1ULL<<8) - 1);\
    ((data) + (index))[3] = (count);

#define set_chunk_mtime(data, index, mtime)\
    assert((mtime) <= (1ULL<<32) - 1);\
    ((data) + (index) + SECTOR_SIZE)[0] = (uint32_t)(mtime) >> (3 * 8);\
    ((data) + (index) + SECTOR_SIZE)[1] = (uint32_t)(mtime) >> (2 * 8);\
    ((data) + (index) + SECTOR_SIZE)[2] = (uint32_t)(mtime) >> (1 * 8);\
    ((data) + (index) + SECTOR_SIZE)[3] = (uint32_t)(mtime) >> (0 * 8);

/// @private
struct anvil_region_file {
    /**
     * file != nullptr indicates the region file is mapped normally.
     *
     * file == nullptr && size == 0 indicates that the region file either does not exist,
     *  or is completely empty. This is completely valid and all chunks have zero length in this case.
     *
     * file == nullptr && size > 0 indicates the region file has been deemed malformed.
     *  The file is unmapped to ensure no writes can occur, and the only valid method on the
     *  region file is to close it.
     */
    char *file;
    size_t size;
    char *path;

    char *chunk_extension; /** the filename extension that chunk files have. */
    int64_t region_x;
    int64_t region_z;
    const anvil_allocator *alloc;

    char *tmp_string;
    size_t tmp_string_cap;

    char *tmp_buffer;
    size_t tmp_buffer_cap;

    int compression_level;
    struct libdeflate_compressor *libdeflate_compressor;
    struct libdeflate_decompressor *libdeflate_decompressor;

#ifdef POSIX
    int fd;
#elifdef WINDOWS
    #error not implemented
#endif

    uint16_t chunk_order[SIZE_X * SIZE_Z];
};

int sort_chunks(const void *a_ptr, const void *b_ptr, void *user) {
    const struct anvil_region_file *region_file = (struct anvil_region_file *)user;
    if (
        region_file->size < HEADER_SIZE ||
        region_file->file == nullptr
    ) {
        return 0;
    }

    const int32_t a_offset = get_chunk_sector_offset(region_file->file, *(uint16_t*)a_ptr);
    const int32_t b_offset = get_chunk_sector_offset(region_file->file, *(uint16_t*)b_ptr);

    return a_offset - b_offset;
}

/**
 * This is essentially a close method, except it attempts to avoid writing any changes
 * that may potentially corrupt the region file even more than it already is.
 *
 * In a perfect world this would roll back the region file to its state before it was opened by this library.
 */
static anvil_result handle_malformed(struct anvil_region_file *region_file) {
    char *file =        region_file->file; region_file->file = nullptr;
    const size_t size = region_file->size; region_file->size = 1;

#ifdef POSIX
    if (msync(file, size, MS_INVALIDATE)) {
        const auto err = errno;
        munmap(file, size);
        errno = err;
        return ANVIL_IO_ERROR;
    }

    if (munmap(file, size)) {
        return ANVIL_IO_ERROR;
    }

    return ANVIL_MALFORMED;

#elifdef WINDOWS
    #error not implemented
#endif
}

static struct libdeflate_decompressor *get_decompressor(
    struct anvil_region_file *region_file
) {
    if (region_file->libdeflate_decompressor == nullptr) {
        struct libdeflate_options opts = {0};
        opts.sizeof_options = sizeof(opts);
        opts.malloc_func = region_file->alloc->malloc;
        opts.free_func = region_file->alloc->free;
        region_file->libdeflate_decompressor = libdeflate_alloc_decompressor_ex(&opts);
    }
    return region_file->libdeflate_decompressor;
}

static struct libdeflate_compressor *get_compressor(
    const int compression_level,
    struct anvil_region_file *region_file
) {
    if (
        region_file->libdeflate_compressor != nullptr &&
        region_file->compression_level != compression_level
    ) {
        libdeflate_free_compressor(region_file->libdeflate_compressor);
        region_file->libdeflate_compressor = nullptr;
    }

    if (region_file->libdeflate_compressor == nullptr) {
        struct libdeflate_options opts = {0};
        opts.sizeof_options = sizeof(opts);
        opts.malloc_func = region_file->alloc->malloc;
        opts.free_func = region_file->alloc->free;
        region_file->libdeflate_compressor = libdeflate_alloc_compressor_ex(compression_level, &opts);
        region_file->compression_level = compression_level;
    }

    return region_file->libdeflate_compressor;
}

anvil_result anvil_region_file_open(
    struct anvil_region_file **region_file_out,
    const char *path,
    const char *chunk_extension,
    const anvil_allocator *alloc
) {
    if (region_file_out == nullptr) return ANVIL_INVALID_USAGE;
    *region_file_out = nullptr;
    if (path == nullptr) return ANVIL_INVALID_USAGE;
    if (alloc == nullptr) {
        alloc = &default_anvil_allocator;
    } else if (
        alloc->malloc == nullptr ||
        alloc->calloc == nullptr ||
        alloc->free == nullptr   ||
        alloc->realloc == nullptr
    ) {
        return ANVIL_INVALID_USAGE;
    }

    if (strlen(path) == 0) {
        return ANVIL_NOT_EXIST;
    }

    int64_t region_x, region_z;
    const anvil_result result = anvil_region_parse_name(path, &region_x, &region_z);
    if (result != ANVIL_OK) {
        return result;
    }

    struct anvil_region_file *region_file = alloc->malloc(sizeof(struct anvil_region_file));
    if (region_file == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

    region_file->file = nullptr;
    region_file->size = 0;
    region_file->path = nullptr;
    region_file->chunk_extension = nullptr;
    region_file->region_x = 0;
    region_file->region_z = 0;
    region_file->alloc = alloc;
    region_file->tmp_string = nullptr;
    region_file->tmp_string_cap = 0;
    region_file->tmp_buffer = nullptr;
    region_file->tmp_buffer_cap = 0;
    region_file->compression_level = 0;
    region_file->libdeflate_compressor = nullptr;
    region_file->libdeflate_decompressor = nullptr;
    for (uint16_t i = 0; i < SIZE_X * SIZE_Z; i++) {
        region_file->chunk_order[i] = i;
    }

    region_file->path = alloc->malloc(strlen(path) + 1);
    if (region_file->path == nullptr) {
        alloc->free(region_file);
        return ANVIL_ALLOC_FAILED;
    }
    strcpy(region_file->path, path);

    if (chunk_extension != nullptr) chunk_extension = "mcc";
    region_file->chunk_extension = alloc->malloc(strlen(chunk_extension) + 1);
    if (region_file->chunk_extension == nullptr) {
        alloc->free(region_file->path);
        alloc->free(region_file);
        return ANVIL_ALLOC_FAILED;
    }
    strcpy(region_file->chunk_extension, chunk_extension);

#ifdef POSIX

    region_file->fd = open(region_file->path, O_RDWR | O_CREAT, 0644);
    if (region_file->fd == -1) {
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

    struct stat st;
    if (fstat(region_file->fd, &st)) {
        const auto err = errno;
        close(region_file->fd);
        errno = err;
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->path);
        alloc->free(region_file);
        return ANVIL_IO_ERROR;
    }

    region_file->size = st.st_size;

    if (region_file->size == 0) {
        region_file->file = nullptr;
        *region_file_out = region_file;
        return ANVIL_OK;
    }

    if (region_file->size < HEADER_SIZE) {
        const auto err = errno;
        close(region_file->fd);
        errno = err;
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->path);
        alloc->free(region_file);
        return ANVIL_MALFORMED;
    }

    region_file->file = mmap(nullptr, region_file->size, PROT_READ | PROT_WRITE, MAP_SHARED, region_file->fd, 0);
    if (region_file->file == MAP_FAILED) {
        const auto err = errno;
        close(region_file->fd);
        errno = err;
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->path);
        alloc->free(region_file);
        return ANVIL_IO_ERROR;
    }

    if (
        region_file->size >= HEADER_SIZE &&
        madvise(region_file->file, HEADER_SIZE, MADV_WILLNEED)
    ) {
        const auto err = errno;
        close(region_file->fd);
        errno = err;
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->path);
        alloc->free(region_file);
        return ANVIL_IO_ERROR;
    }

    qsort_r()

    *region_file_out = region_file;
    return ANVIL_OK;

#elifdef WINDOWS
#error not implemented
#endif
}

uint32_t anvil_chunk_mtime(
    const struct anvil_region_file *region_file,
    const int64_t chunk_x,
    const int64_t chunk_z
) {
    if (
        region_file == nullptr ||
        region_file->size < HEADER_SIZE
    ) {
        return 0;
    }

    return get_chunk_mtime(region_file->file, chunk_x, chunk_z);
}

anvil_result decompress(
    struct anvil_region_file *region_file,
    const unsigned char compression_type,
    const char *in,
    const size_t in_len,
    char *out,
    const size_t out_cap,
    size_t *out_len
) {
    switch (compression_type) {
    case ANVIL_COMPRESSION_NONE: {
        if (out_cap < in_len) {
            *out_len = in_len;
            return ANVIL_INSUFFICIENT_SPACE;
        }

        memcpy(out, in, in_len);
        *out_len = in_len;
        return ANVIL_OK;
    }
    case ANVIL_COMPRESSION_GZIP: {
        struct libdeflate_decompressor *decompressor = get_decompressor(region_file);
        if (decompressor == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

        const enum libdeflate_result res = libdeflate_gzip_decompress(
            decompressor,
            in,
            in_len,
            out,
            out_cap,
            out_len
        );

        switch (res) {
        case LIBDEFLATE_SUCCESS: return ANVIL_OK;
        case LIBDEFLATE_BAD_DATA: return ANVIL_MALFORMED;
        case LIBDEFLATE_INSUFFICIENT_SPACE: {
            auto guess = (out_cap << 1) - (out_cap >> 1);
            if (guess == 0) guess = SECTOR_SIZE;
            if (guess < in_len) guess = in_len * 2;
            *out_len = guess;
            return ANVIL_INSUFFICIENT_SPACE;
        }
        case LIBDEFLATE_SHORT_OUTPUT: return ANVIL_OK;
        }
    }
    case ANVIL_COMPRESSION_ZLIB: {
        struct libdeflate_decompressor *decompressor = get_decompressor(region_file);
        if (decompressor == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

        const enum libdeflate_result res = libdeflate_gzip_decompress(
            decompressor,
            in,
            in_len,
            out,
            out_cap,
            out_len
        );

        switch (res) {
        case LIBDEFLATE_SUCCESS: return ANVIL_OK;
        case LIBDEFLATE_BAD_DATA: return ANVIL_MALFORMED;
        case LIBDEFLATE_INSUFFICIENT_SPACE: {
            auto guess = (out_cap << 1) - (out_cap >> 1);
            if (guess == 0) guess = SECTOR_SIZE;
            if (guess < in_len) guess = in_len * 2;
            *out_len = guess;
            return ANVIL_INSUFFICIENT_SPACE;
        }
        case LIBDEFLATE_SHORT_OUTPUT: return ANVIL_OK;
        }
    }
    default: {
        return ANVIL_UNSUPPORTED_COMPRESSION;
    }
    }
}

anvil_result anvil_chunk_read(
    void *restrict out,
    const size_t out_cap,
    size_t *out_len,
    const int64_t chunk_x,
    const int64_t chunk_z,
    struct anvil_region_file *region_file
) {
    if (region_file == nullptr) return ANVIL_INVALID_USAGE;
    if (region_file->file == nullptr) {
        if (region_file->size > 0) return ANVIL_INVALID_USAGE;

        if (out_len != nullptr) *out_len = 0;
        return ANVIL_OK;
    }

    assert(region_file->size >= HEADER_SIZE);

    const size_t sector_offset = get_chunk_sector_offset(region_file->file, chunk_index(chunk_x, chunk_z));
    const size_t sector_count = get_chunk_sector_count(region_file->file, chunk_index(chunk_x, chunk_z));

    if (sector_offset * SECTOR_SIZE + 5 > region_file->size) {
        return handle_malformed(region_file);
    }

    char *cursor = region_file->file + sector_offset * SECTOR_SIZE;

    size_t chunk_size =
        cursor[0] << 24 |
        cursor[1] << 16 |
        cursor[2] << 8  |
        cursor[3] - 1   ;
    const uint8_t compression_type =
        cursor[4] & 0b01111111;
    const bool separate_file =
        (cursor[4] & 0b10000000) > 0;

    if (separate_file) {
    try_chunk_name_again:
        const int path_len = anvil_region_filename(
            region_file->tmp_string,
            region_file->tmp_string_cap,
            "c",
            region_file->region_x * SIZE_X + mod(chunk_x, SIZE_X),
            region_file->region_z * SIZE_Z + mod(chunk_x, SIZE_Z),
            region_file->chunk_extension
        );

        if (path_len >= region_file->tmp_string_cap) {
            char *new = region_file->alloc->realloc(region_file->tmp_string, path_len);
            if (new == nullptr) return ANVIL_ALLOC_FAILED;

            region_file->tmp_string = new;
            region_file->tmp_string_cap = path_len;
            goto try_chunk_name_again;
        }

#ifdef POSIX

        const int fd = open(region_file->tmp_string, O_RDONLY);
        if (fd == -1) {
            switch (errno) {
            case ENOENT:
                errno = 0;
                if (out_len != nullptr) *out_len = 0;
                return ANVIL_OK;
            case ENOMEM: return ANVIL_ALLOC_FAILED;
            case ENOTDIR: return ANVIL_NOT_EXIST;
            default: return ANVIL_IO_ERROR;
            }
        }

        struct stat st;
        if (fstat(fd, &st)) {
            const auto err = errno;
            close(fd);
            errno = err;
            return ANVIL_IO_ERROR;
        }

        cursor = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (cursor == MAP_FAILED) {
            const auto err = errno;
            close(fd);
            errno = err;
            return ANVIL_IO_ERROR;
        }

        if (close(fd)) {
            const auto err = errno;
            munmap(cursor, st.st_size);
            errno = err;
            return ANVIL_IO_ERROR;
        }

        chunk_size = st.st_size;

#elifdef WINDOWS
#error not implemented
#endif

    } else {
        if (
            sector_offset * SECTOR_SIZE + chunk_size > region_file->size ||
            chunk_size > sector_count * SECTOR_SIZE
        ) {
            return handle_malformed(region_file);
        }

        if (chunk_size == 0) {
            if (out_len != nullptr) *out_len = 0;
            return ANVIL_OK;
        }

        cursor = region_file->file + sector_offset * SECTOR_SIZE + 5;
    }

    const anvil_result res = decompress(
        region_file,
        compression_type,
        cursor,
        chunk_size,
        out,
        out_cap,
        out_len
    );

#ifdef POSIX

    if (separate_file && munmap(cursor, chunk_size)) {
        if (res != ANVIL_OK) {
            return res;
        }
        return ANVIL_IO_ERROR;
    }

#else
#error not implemented
#endif

    return res;
}

// finds a place to write chunk data.
static anvil_result make_chunk_space(
    struct anvil_region_file *region_file,
    char **cursor,
    const int64_t chunk_x,
    const int64_t chunk_z,
    const size_t new_size
) {
    assert(new_size < 1 * 1024 * 1024);

    for (int64_t i = 0; i < SIZE_X * SIZE_Z; i++) {

    }

    const char *cursor = region_file->file + ((chunk_x & 31) * 32 + (chunk_z & 31)) * 4;

    const size_t sector_offset =
        (unsigned char)cursor[0] << (2 * 8) |
        (unsigned char)cursor[1] << (1 * 8) |
        (unsigned char)cursor[2] << (0 * 8) ;

    const size_t sector_count =
        (unsigned char)cursor[3];
}

/**
 * in the event of an error, the region file shall be unmodified
 * except the region [pos, section_old_size) which may have undefined contents.
 */
static anvil_result resize(
    struct anvil_region_file *region_file,
    const size_t pos,
    const size_t section_old_size,
    const size_t section_new_size
) {
    assert(region_file->size >= pos + section_old_size);
    const size_t file_new_size = region_file->size + section_new_size - section_old_size;

    if (section_new_size < section_old_size && pos + section_old_size < region_file->size) {
        memmove(
            region_file->file + pos + section_new_size,
            region_file->file + pos + section_old_size,
            region_file->size - pos - section_old_size
        );
    }

#ifdef POSIX

    if (ftruncate(region_file->fd, (off_t)file_new_size)) {

        if (section_new_size < section_old_size && pos + section_old_size < region_file->size) {
            memmove(
                region_file->file + pos + section_old_size,
                region_file->file + pos + section_new_size,
                region_file->size - pos - section_old_size
            );
        }

        return ANVIL_IO_ERROR;
    }

    #ifdef OS_HAS_GNU_SOURCE

        char *new = os_mremap(region_file->file, region_file->size, file_new_size, OS_MREMAP_MAYMOVE);
        if (new == MAP_FAILED) {
            if (section_new_size < section_old_size && pos + section_old_size < region_file->size) {
                memmove(
                    region_file->file + pos + section_old_size,
                    region_file->file + pos + section_new_size,
                    region_file->size - pos - section_old_size
                );
            }

            return ANVIL_IO_ERROR;
        }

        // wow, wasn't that easy and efficient.
        // I am *not* looking forward to trying to figure out how to do that on other systems.
        // I am disappointed that darwin and bsd seem to be missing a remapping method.
        //
        // It would be unfortunate if we end up wasting huge amounts
        // of time unnecessarily syncing half-written region files to the disk.
        region_file->size = file_new_size;
        region_file->file = new;

    #else



    #endif


#elifdef WINDOWS

#endif


}

anvil_result anvil_chunk_write(
    const void *restrict in,
    size_t in_len,
    double compression_level,
    enum anvil_compression compression,
    int64_t chunk_x,
    int64_t chunk_z,
    struct anvil_region_file *region_file
) {
    if (
        region_file == nullptr ||
        (in_len > 0 && in == nullptr) ||
        (region_file->file == nullptr && region_file->size > 0)
    ) {
        return ANVIL_INVALID_USAGE;
    }

    const char *restrict chunk_data;
    size_t chunk_size;

    switch (compression) {
    case ANVIL_COMPRESSION_NONE: {
        chunk_data = in;
        chunk_size = in_len;
        break;
    }
    case ANVIL_COMPRESSION_GZIP: {
        struct libdeflate_compressor *compressor = get_compressor((int)(compression_level * 12.0), region_file);
        if (compressor == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

        const size_t max_size = libdeflate_gzip_compress_bound(compressor, in_len);
        if (region_file->tmp_buffer_cap < max_size) {
            char *new = region_file->alloc->realloc(region_file->tmp_buffer, max_size);
            if (new == nullptr) {
                return ANVIL_ALLOC_FAILED;
            }

            region_file->tmp_buffer = new;
            region_file->tmp_buffer_cap = max_size;
        }

        chunk_size = libdeflate_gzip_compress(
            compressor,
            in,
            in_len,
            region_file->tmp_buffer,
            region_file->tmp_buffer_cap
        );

        // this should never happen as we grew the buffer to the maximum theoretical size.
        // compression doesn't have any other failure modes (perhaps except SIGBUS/SIGSEGV).
        assert(chunk_size > 0);

        chunk_data = region_file->tmp_buffer;
        break;
    }
    case ANVIL_COMPRESSION_ZLIB: {
        struct libdeflate_compressor *compressor = get_compressor((int)(compression_level * 12.0), region_file);
        if (compressor == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

        const size_t max_size = libdeflate_zlib_compress_bound(compressor, in_len);
        if (region_file->tmp_buffer_cap < max_size) {
            char *new = region_file->alloc->realloc(region_file->tmp_buffer, max_size);
            if (new == nullptr) {
                return ANVIL_ALLOC_FAILED;
            }

            region_file->tmp_buffer = new;
            region_file->tmp_buffer_cap = max_size;
        }

        chunk_size = libdeflate_zlib_compress(
            compressor,
            in,
            in_len,
            region_file->tmp_buffer,
            region_file->tmp_buffer_cap
        );

        // this should never happen as we grew the buffer to the maximum theoretical size.
        // compression doesn't have any other failure modes (perhaps except SIGBUS/SIGSEGV).
        assert(chunk_size > 0);

        chunk_data = region_file->tmp_buffer;
        break;
    }
    default: return ANVIL_UNSUPPORTED_COMPRESSION;
    }

    if (region_file->size < HEADER_SIZE) {

#ifdef POSIX



#elifdef WINDOWS

#endif


    }

    char *cursor = region_file->file + ((chunk_x & 31) * 32 + (chunk_z & 31)) * 4;

    const size_t sector_offset =
    (unsigned char)cursor[0] << (2 * 8) |
    (unsigned char)cursor[1] << (1 * 8) |
    (unsigned char)cursor[2] << (0 * 8) ;

    const size_t sector_count =
        (unsigned char)cursor[3];

    if (sector_offset * SECTOR_SIZE + 5 > region_file->size) {
        return handle_malformed(region_file);
    }

    cursor = region_file->file + sector_offset * SECTOR_SIZE;
}

anvil_result anvil_region_file_close(struct anvil_region_file *region_file) {
    region_file->alloc->free(region_file->chunk_extension);
    region_file->alloc->free(region_file->path);
    if (region_file->tmp_string != nullptr)
        region_file->alloc->free(region_file->tmp_string);
    if (region_file->tmp_buffer != nullptr)
        region_file->alloc->free(region_file->tmp_buffer);
    if (region_file->libdeflate_compressor != nullptr)
        libdeflate_free_compressor(region_file->libdeflate_compressor);
    if (region_file->libdeflate_decompressor != nullptr)
        libdeflate_free_decompressor(region_file->libdeflate_decompressor);

#ifdef POSIX

    if (
        region_file->file != nullptr &&
        munmap(region_file->file, region_file->size)
    ) {
        const auto err = errno;
        close(region_file->fd);
        errno = err;
        return ANVIL_IO_ERROR;
    }

    unlinkat()

    if (close(region_file->fd)) {
        return ANVIL_IO_ERROR;
    }

#elifdef WINDOWS
#error not implemented
#endif



    region_file->alloc->free(region_file);
    return ANVIL_OK;
}
