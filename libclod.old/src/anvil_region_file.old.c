#include <errno.h>
#include <stdint.h>
#include <libdeflate.h>

#include "anvil.h"
#include "anvil_internal.h"
#include "buffer.h"

#ifdef CLOD_USE_POSIX

#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#else
#error not implemented
#endif

#define SIZE_X 32
#define SIZE_Z 32
#define SECTOR_SIZE 4096

#define LOCATION_HEADER_OFFSET 0
#define LOCATION_HEADER_SIZE (SIZE_X * SIZE_Z * 4)

#define MTIME_HEADER_OFFSET LOCATION_HEADER_SIZE
#define MTIME_HEADER_SIZE (SIZE_X * SIZE_Z * 4)

#define HEADER_SECTORS ((LOCATION_HEADER_SIZE + MTIME_HEADER_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE)

#define mod(a, b)\
    ((((a) % (b)) + (b)) % (b))

#define chunk_index(chunk_x, chunk_z)\
    (mod((chunk_x), SIZE_X) * SIZE_Z + mod((chunk_z), SIZE_Z))

#define get_sector_offset(data, index) (\
    (uint32_t)(unsigned char)((data) + (index * 4) + LOCATION_HEADER_OFFSET)[0] << (2 * 8) |\
    (uint32_t)(unsigned char)((data) + (index * 4) + LOCATION_HEADER_OFFSET)[1] << (1 * 8) |\
    (uint32_t)(unsigned char)((data) + (index * 4) + LOCATION_HEADER_OFFSET)[2] << (0 * 8) )\

#define get_sector_count(data, index) (\
    (uint8_t)(unsigned char)((data) + (index * 4) + LOCATION_HEADER_OFFSET)[3])

#define get_mtime(data, index) (\
    (uint32_t)(unsigned char)((data) + (index * 4) + MTIME_HEADER_OFFSET)[0] << (3 * 8) |\
    (uint32_t)(unsigned char)((data) + (index * 4) + MTIME_HEADER_OFFSET)[1] << (2 * 8) |\
    (uint32_t)(unsigned char)((data) + (index * 4) + MTIME_HEADER_OFFSET)[2] << (1 * 8) |\
    (uint32_t)(unsigned char)((data) + (index * 4) + MTIME_HEADER_OFFSET)[3] << (0 * 8) )\

#define set_sector_offset(data, index, offset)\
    ((data) + (index * 4) + LOCATION_HEADER_OFFSET)[0] = (char)(uint32_t)(offset) >> (2 * 8);\
    ((data) + (index * 4) + LOCATION_HEADER_OFFSET)[1] = (char)(uint32_t)(offset) >> (1 * 8);\
    ((data) + (index * 4) + LOCATION_HEADER_OFFSET)[2] = (char)(uint32_t)(offset) >> (0 * 8);

#define set_sector_count(data, index, count) \
    ((data) + (index * 4) + LOCATION_HEADER_OFFSET)[3] = (char)(count);

#define set_mtime(data, index, mtime)\
    ((data) + (index * 4) + MTIME_HEADER_OFFSET)[0] = (char)(uint32_t)(mtime) >> (3 * 8);\
    ((data) + (index * 4) + MTIME_HEADER_OFFSET)[1] = (char)(uint32_t)(mtime) >> (2 * 8);\
    ((data) + (index * 4) + MTIME_HEADER_OFFSET)[2] = (char)(uint32_t)(mtime) >> (1 * 8);\
    ((data) + (index * 4) + MTIME_HEADER_OFFSET)[3] = (char)(uint32_t)(mtime) >> (0 * 8);

#define get_end_sector(data, indices) (\
    sort_indices((data), (indices)),\
    get_sector_offset((data), (indices)[SIZE_X * SIZE_Z - 1]) +\
    get_sector_count((data), (indices)[SIZE_X * SIZE_Z - 1])\
)

/// @private
struct anvil_file {
    /**
     * file != nullptr && assert(size > HEADER_SIZE) indicates the region file is mapped normally.
     *
     * file == nullptr && size == 0 indicates that the region file either does not exist,
     *  or is completely empty. This is completely valid and all chunks have zero length in this case.
     *
     * file == nullptr && size > 0 indicates the region file has been deemed malformed.
     *  The file is unmapped to ensure no writes can occur, and the only valid method on the
     *  region file is to close it.
     */
    char *restrict file;
    size_t size;

    char *chunk_extension; /** the filename extension that chunk files have. */
    int64_t *coords;
    const anvil_allocator *alloc;

    char *tmp_string;
    size_t tmp_string_cap;

    char *tmp_buffer;
    size_t tmp_buffer_cap;

    int compression_level;
    struct libdeflate_compressor *libdeflate_compressor;
    struct libdeflate_decompressor *libdeflate_decompressor;

#ifdef CLOD_USE_POSIX
    int fd;
    int dir_fd;
#else
    #error not implemented
#endif

    /**
     * the index of chunks as they appear in the region file.
     * i.e. if the first bit of data in the file is for chunk 4,2 then
     * chunk_indices[0] = 4 * SIZE_Z + 2
     */
    uint16_t *chunk_indices;
};

/**
 * This is essentially a close method, except it attempts to avoid writing any changes
 * that may potentially corrupt the region file even more than it already is.
 *
 * In a perfect world this would roll back the region file to its state before it was opened by this library.
 */
static anvil_result handle_malformed(anvil_file *region_file) {
    char *file =        region_file->file; region_file->file = nullptr;
    const size_t size = region_file->size; region_file->size = 1;

#ifdef CLOD_USE_POSIX

    if (msync(file, size, MS_INVALIDATE)) {
        const auto err = errno;
        munmap(file, size);
        anvil_errno = anvil_errno_get(err, errno = err);
        return anvil_errno;
    }

    if (munmap(file, size)) {
        anvil_errno = anvil_errno_get(errno);
        return anvil_errno;
    }



    return ANVIL_MALFORMED;

#else
    #error not implemented
#endif
}

static uint16_t *get_indices(anvil_file *region_file) {
    if (region_file->chunk_indices == nullptr) {
        region_file->chunk_indices = region_file->alloc->malloc(sizeof (uint16_t) * SIZE_X * SIZE_Z);
        if (region_file->chunk_indices == nullptr) {
            return nullptr;
        }

        for (int64_t i = 0; i < SIZE_X * SIZE_Z; i++) {
            region_file->chunk_indices[i] = (uint16_t)i;
        }
    }

    return region_file->chunk_indices;
}

static void sort_indices(
    const char *restrict data,
    uint16_t *restrict indices
) {
    int64_t unsorted = SIZE_X * SIZE_Z;
    do {
        int64_t last_swap = 0;
        for (int64_t i = 1; i < unsorted; i++) {
            const int64_t a_offset = get_sector_offset(data, indices[i - 1]);
            const int64_t a_count = get_sector_count(data, indices[i - 1]);
            const int64_t b_offset = get_sector_offset(data, indices[i]);
            const int64_t b_count = get_sector_count(data, indices[i]);

            if (a_offset > b_offset || (a_offset == b_offset && a_count > b_count)) {
                const int64_t tmp = indices[i];
                indices[i] = indices[i - 1];
                indices[i - 1] = tmp;
                last_swap = i;
            }
        }
        unsorted = last_swap;
    } while (unsorted > 1);
}

static struct libdeflate_decompressor *get_decompressor(
    anvil_file *region_file
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
    anvil_file *region_file
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

char *get_chunk_filename(
    anvil_file *region_file,
    const int64_t chunk_x,
    const int64_t chunk_z
) {
try_chunk_name_again:
    const size_t path_len = anvil_create_filename(
        region_file->tmp_string,
        region_file->tmp_string_cap,
        "c",
        region_file->chunk_extension,
        2,
        region_file->region_x * SIZE_X + mod(chunk_x, SIZE_X),
        region_file->region_z * SIZE_Z + mod(chunk_z, SIZE_Z)
    );

    if (path_len >= region_file->tmp_string_cap) {
        char *new = region_file->alloc->realloc(region_file->tmp_string, path_len);
        if (new == nullptr) return nullptr;

        region_file->tmp_string = new;
        region_file->tmp_string_cap = path_len;
        goto try_chunk_name_again;
    }

    return region_file->tmp_string;
}

anvil_file *anvil_region_file_openat(
    const int64_t region_x,
    const int64_t region_z,
    const char *chunk_extension,
    const char *region_extension,

#ifdef CLOD_USE_POSIX
    const int dir_fd,
#else
#error not implemented
#endif

    const anvil_allocator *alloc
) {
    if (alloc == nullptr) {
        alloc = &default_anvil_allocator;
    }

    anvil_assert(alloc->malloc != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);
    anvil_assert(alloc->calloc != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);
    anvil_assert(alloc->free != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);
    anvil_assert(alloc->realloc != nullptr, anvil_errno = ANVIL_INVALID_USAGE; return nullptr);

    anvil_file *region_file = alloc->malloc(sizeof(anvil_file));
    if (region_file == nullptr) {
        anvil_errno = ANVIL_ALLOC_FAILED;
        return nullptr;
    }

    region_file->file = nullptr;
    region_file->size = 0;
    region_file->chunk_extension = nullptr;
    region_file->region_x = region_x;
    region_file->region_z = region_z;
    region_file->alloc = alloc;
    region_file->tmp_string = nullptr;
    region_file->tmp_string_cap = 0;
    region_file->tmp_buffer = nullptr;
    region_file->tmp_buffer_cap = 0;
    region_file->compression_level = 0;
    region_file->libdeflate_compressor = nullptr;
    region_file->libdeflate_decompressor = nullptr;
    region_file->chunk_indices = nullptr;

    if (chunk_extension != nullptr) chunk_extension = "mcc";
    region_file->chunk_extension = string_copy(alloc->malloc, chunk_extension);
    if (region_file->chunk_extension == nullptr) {
        alloc->free(region_file);
        anvil_errno = ANVIL_ALLOC_FAILED;
        return nullptr;
    }

    const auto name_len = anvil_create_filename(
        nullptr,
        0,
        "r",
        region_extension,
        2,
        region_x,
        region_z
    );
    region_file->tmp_string = alloc->malloc(name_len);
    if (region_file->tmp_string == nullptr) {
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file);
        anvil_errno = ANVIL_ALLOC_FAILED;
        return nullptr;
    }
    region_file->tmp_string_cap = name_len;
    anvil_create_filename(
        region_file->tmp_string,
        region_file->tmp_string_cap,
        "r",
        region_extension,
        2,
        region_x,
        region_z
    );

#ifdef CLOD_USE_POSIX

    region_file->dir_fd = fcntl(dir_fd, F_DUPFD);
    if (region_file->dir_fd < 0) {
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->tmp_string);
        alloc->free(region_file);
        anvil_errno = anvil_errno_get(errno);
        return nullptr;
    }

    region_file->fd = openat(dir_fd, region_file->tmp_string,  O_RDWR | O_CREAT, 0644);
    if (region_file->fd == -1) {
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->tmp_string);
        alloc->free(region_file);
        anvil_errno = anvil_errno_get(errno);
        return nullptr;
    }

    struct stat st;
    if (fstat(region_file->fd, &st)) {
        const auto err = errno;
        close(region_file->fd);
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->tmp_string);
        alloc->free(region_file);
        errno = err;
        anvil_errno = anvil_errno_get(errno);
        return nullptr;
    }

    region_file->size = st.st_size;

    if (region_file->size == 0) {
        region_file->file = nullptr;
        return region_file;
    }

    if (region_file->size < HEADER_SECTORS * SECTOR_SIZE) {
        if (close(region_file->fd)) {
            alloc->free(region_file->chunk_extension);
            alloc->free(region_file->tmp_string);
            alloc->free(region_file);
            anvil_errno = anvil_errno_get(errno);
            return nullptr;
        }
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->tmp_string);
        alloc->free(region_file);
        anvil_errno = ANVIL_MALFORMED;
        return nullptr;
    }

    region_file->file = mmap(
        nullptr,
        region_file->size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        region_file->fd,
        0
    );
    if (region_file->file == MAP_FAILED) {
        const auto err = errno;
        close(region_file->fd);
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->tmp_string);
        alloc->free(region_file);
        anvil_errno = anvil_errno_get(err, errno = err);
        return nullptr;
    }

    if (
        region_file->size >= HEADER_SECTORS * SECTOR_SIZE &&
        madvise(region_file->file, HEADER_SECTORS * SECTOR_SIZE, MADV_WILLNEED)
    ) {
        const auto err = errno;
        close(region_file->fd);
        alloc->free(region_file->chunk_extension);
        alloc->free(region_file->tmp_string);
        alloc->free(region_file);
        anvil_errno = anvil_errno_get(err, errno = err);
        return nullptr;
    }

    return region_file;

#else
#error not implemented
#endif

}

uint32_t anvil_mtime(
    const anvil_file *file,
    const int64_t chunk_x,
    const int64_t chunk_z
) {
    if (
        file == nullptr ||
        file->size < HEADER_SECTORS * SECTOR_SIZE
    ) {
        return 0;
    }

    return get_mtime(file->file, chunk_index(chunk_x, chunk_z));
}

anvil_result decompress(
    anvil_file *region_file,
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
            return ANVIL_TOO_SMALL;
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
            return ANVIL_TOO_SMALL;
        }
        case LIBDEFLATE_SHORT_OUTPUT: return ANVIL_OK;
        }
    }
    case ANVIL_COMPRESSION_ZLIB: {
        struct libdeflate_decompressor *decompressor = get_decompressor(region_file);
        if (decompressor == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

        const enum libdeflate_result res = libdeflate_zlib_decompress(
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
            return ANVIL_TOO_SMALL;
        }
        case LIBDEFLATE_SHORT_OUTPUT: return ANVIL_OK;
        }
    }
    default: {
        return ANVIL_UNSUPPORTED_COMPRESSION;
    }
    }
}

anvil_result anvil_read(
    void *restrict out,
    const size_t out_cap,
    size_t *out_len,
    const int64_t chunk_x,
    const int64_t chunk_z,
    anvil_file *file
) {
    anvil_assert(file != nullptr, return anvil_errno = ANVIL_INVALID_USAGE);
    anvil_assert(file->file != nullptr || file->size == 0, return anvil_errno = ANVIL_INVALID_USAGE);

    if (file->size == 0) {
        if (out_len != nullptr) *out_len = 0;
        return ANVIL_OK;
    }

    anvil_assert(file->size >= HEADER_SECTORS * SECTOR_SIZE, return anvil_errno = ANVIL_MALFORMED);

    const size_t sector_offset = get_sector_offset(file->file, chunk_index(chunk_x, chunk_z));
    const size_t sector_count = get_sector_count(file->file, chunk_index(chunk_x, chunk_z));

    if (sector_offset * SECTOR_SIZE + 5 > file->size) {
        return handle_malformed(file);
    }

    char *restrict cursor = file->file + sector_offset * SECTOR_SIZE;

    size_t chunk_size =(
        ((uint8_t)cursor[0] << (3 * 8)) +
        ((uint8_t)cursor[1] << (2 * 8)) +
        ((uint8_t)cursor[2] << (1 * 8)) +
        ((uint8_t)cursor[3] << (0 * 8))) - 1;
    const uint8_t compression_type =
        cursor[4] & 0b01111111;
    const bool separate_file =
        (cursor[4] & 0b10000000) > 0;

    if (separate_file) {
        const char *chunk_filename = get_chunk_filename(file, chunk_x, chunk_z);
        if (chunk_filename == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

#ifdef CLOD_USE_POSIX

        const int fd = openat(file->dir_fd, chunk_filename, O_RDONLY);
        if (fd == -1) {
            if (errno == ENOENT) {
                errno = 0;
                if (out_len != nullptr) *out_len = 0;
                return ANVIL_OK;
            }

            return anvil_errno = anvil_errno_get(errno);
        }

        struct stat st;
        if (fstat(fd, &st)) {
            const auto err = errno;
            close(fd);
            return anvil_errno = anvil_errno_get(err, errno = err);
        }

        chunk_size = st.st_size;

        cursor = mmap(nullptr, chunk_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (cursor == MAP_FAILED) {
            const auto err = errno;
            close(fd);
            return anvil_errno = anvil_errno_get(err, errno = err);
        }

        if (close(fd)) {
            const auto err = errno;
            munmap(cursor, chunk_size);
            return anvil_errno = anvil_errno_get(err, errno = err);
        }

#else
#error not implemented
#endif

    } else {
        if (
            sector_offset * SECTOR_SIZE + chunk_size > file->size ||
            chunk_size > sector_count * SECTOR_SIZE
        ) {
            return handle_malformed(file);
        }

        if (chunk_size == 0) {
            if (out_len != nullptr) *out_len = 0;
            return ANVIL_OK;
        }

        cursor = file->file + sector_offset * SECTOR_SIZE + 5;
    }

    const anvil_result res = decompress(
        file,
        compression_type,
        cursor,
        chunk_size,
        out,
        out_cap,
        out_len
    );

#ifdef CLOD_USE_POSIX

    if (res != ANVIL_OK) {
        const auto err = errno;
        if (separate_file) {
            munmap(cursor, chunk_size);
        }
        errno = err;
        return res;
    }

    if (separate_file && munmap(cursor, chunk_size)) {
        return anvil_errno = anvil_errno_get(errno);
    }

#else
#error not implemented
#endif

    return res;
}

/**
 * resize the file to the new required size.
 */
static anvil_result realloc_region_file(
    anvil_file *region_file,
    const size_t sectors
) {
    anvil_assert(region_file != nullptr, return anvil_errno = ANVIL_INVALID_USAGE);
    anvil_assert(region_file->file != nullptr || region_file->size == 0, return anvil_errno = ANVIL_INVALID_USAGE);
    anvil_assert(sectors == 0 || sectors >= HEADER_SECTORS, return anvil_errno = ANVIL_INVALID_USAGE);

    if (sectors * SECTOR_SIZE == region_file->size) {
        return ANVIL_OK;
    }

    const auto old_size = region_file->size;

#ifdef CLOD_USE_POSIX

    if (ftruncate(region_file->fd, (off_t)sectors * SECTOR_SIZE)) {
        return anvil_errno = anvil_errno_get(errno);
    }

    if (sectors * SECTOR_SIZE < old_size) {
        if (munmap(region_file->file + sectors * SECTOR_SIZE, old_size - sectors * SECTOR_SIZE)) {
            return anvil_errno = anvil_errno_get(errno);
        }

        region_file->size = sectors * SECTOR_SIZE;
        if (sectors == 0) region_file->file = nullptr;
        return ANVIL_OK;
    }

    #ifndef _GNU_SOURCE

        if (region_file->file != nullptr) {
            if (munmap(region_file->file, old_size)) {
                return ANVIL_IO_ERROR;
            }

            region_file->size = 0;
            region_file->file = nullptr;
        }

    #endif

    if (region_file->file == nullptr) {
        region_file->file = mmap(
            nullptr,
            sectors * SECTOR_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            region_file->fd,
            0
        );
        if (region_file->file == MAP_FAILED) {
            region_file->file = nullptr;
            return anvil_errno = anvil_errno_get(errno);
        }

        region_file->size = sectors * SECTOR_SIZE;

        if (
            old_size >= HEADER_SECTORS * SECTOR_SIZE &&
            madvise(region_file->file, HEADER_SECTORS * SECTOR_SIZE, MADV_WILLNEED)
        ) {
            return anvil_errno = anvil_errno_get(errno);
        }

        return ANVIL_OK;
    }

    #ifdef _GNU_SOURCE

        char *new = mremap(region_file->file, old_size, sectors * SECTOR_SIZE, MREMAP_MAYMOVE);
        if (new == MAP_FAILED) {
            return anvil_errno = anvil_errno_get(errno);
        }

        // wow, wasn't that easy and efficient.
        // I am *not* looking forward to trying to figure out how to do that on other systems.
        // I am disappointed that darwin and bsd seem to be missing a remapping method.
        //
        // It would be unfortunate if we end up wasting huge amounts
        // of time unnecessarily syncing half-complete region files to the disk.
        region_file->file = new;

    #endif


#else
    #error not implemented
#endif
    return ANVIL_OK;
}

// creates space somewhere to store the chunk data
// while making a best effort attempt at avoiding any possibility for corruption.
static anvil_result chunk_realloc(
    anvil_file *region_file,
    char **cursor,
    const int64_t chunk_x,
    const int64_t chunk_z,
    const size_t sectors
) {
    anvil_assert(region_file != nullptr, return anvil_errno = ANVIL_INVALID_USAGE);
    anvil_assert(region_file->file != nullptr || region_file->size == 0, return anvil_errno = ANVIL_INVALID_USAGE);
    anvil_assert(sectors == 0 || cursor != nullptr, return anvil_errno = ANVIL_INVALID_USAGE);
    anvil_assert(sectors <= UINT8_MAX, return anvil_errno = ANVIL_INVALID_USAGE);

    // we need to allocate no space from nothing.
    // perfect!
    if (region_file->size == 0 && sectors == 0) {
        *cursor = nullptr;
        return ANVIL_OK;
    }

    // we need to allocate space from nothing.
    // initialise the region file.
    if (region_file->size == 0) {
        // or actually get someone else to initialise the region file.
        const anvil_result ret = realloc_region_file(region_file, HEADER_SECTORS + sectors);
        if (ret != ANVIL_OK) {
            return ret;
        }

        set_sector_count(region_file->file, chunk_index(chunk_x, chunk_z), sectors);
        set_sector_offset(region_file->file, chunk_index(chunk_x, chunk_z), HEADER_SECTORS * SECTOR_SIZE);
        *cursor = region_file->file + HEADER_SECTORS * SECTOR_SIZE;
        return ANVIL_OK;
    }

    // there are existing chunks in this region file,
    // so we need to get setup to search for space in it.

    uint16_t *chunk_indices = get_indices(region_file);
    if (chunk_indices == nullptr) {
        return ANVIL_ALLOC_FAILED;
    }

    // we need to allocate no space from existing space.
    if (sectors == 0) {
        set_sector_offset(region_file->file, chunk_index(chunk_x, chunk_z), 0);
        set_sector_count(region_file->file, chunk_index(chunk_x, chunk_z), 0);
        *cursor = nullptr;

        // maybe shrink the file, but wait!
        // If this is the last chunk in the region file, we want to delete the whole region file too!
        const size_t end_sector = get_end_sector(region_file->file, chunk_indices);
        if (end_sector == HEADER_SECTORS) {
            return realloc_region_file(region_file, 0);
        }
        return realloc_region_file(region_file, end_sector);
    }

    // the general idea here is to search for any free space in the file,
    // and taking the first area of free space that's large enough.
    //
    // notably, this method does not attempt to replace chunk data in-place.
    // this is because doing so would prevent the file from ever shrinking
    // unless the last chunks re-grew enough to trigger a search for more space
    // which seems like a counterproductive process for shrinking.

    sort_indices(region_file->file, chunk_indices);

    int64_t a_index = 0;
    while (a_index < SIZE_X * SIZE_Z && get_sector_count(region_file->file, chunk_indices[a_index]) == 0) a_index++;
    for (int64_t b_index = a_index + 1; b_index < SIZE_X * SIZE_Z; a_index = b_index, b_index++) {
        while (b_index < SIZE_X * SIZE_Z && get_sector_count(region_file->file, chunk_indices[b_index]) == 0) b_index++;

        const int64_t a_offset = get_sector_offset(region_file->file, chunk_indices[a_index]);
        const int64_t a_count = get_sector_count(region_file->file, chunk_indices[a_index]);
        const int64_t b_offset = get_sector_offset(region_file->file, chunk_indices[b_index]);

        const int64_t free_count = b_offset - a_offset - a_count;

        if (free_count >= sectors) {
            set_sector_count(region_file->file, chunk_index(chunk_x, chunk_z), sectors);
            set_sector_offset(region_file->file, chunk_index(chunk_x, chunk_z), a_offset + a_count);
            *cursor = region_file->file + (a_offset + a_count) * SECTOR_SIZE;

            const size_t end_sector = get_end_sector(region_file->file, chunk_indices);

            // maybe shrink the file
            return realloc_region_file(region_file, end_sector);
        }
    }

    // no free space found.
    // need to grow the file.

    const auto end_sector = get_end_sector(region_file->file, chunk_indices);

    *cursor = region_file->file + end_sector * SECTOR_SIZE;

    set_sector_count(region_file->file, chunk_index(chunk_x, chunk_z), sectors);
    set_sector_offset(region_file->file, chunk_index(chunk_x, chunk_z), end_sector);

    return realloc_region_file(region_file, end_sector + sectors);
}

anvil_result anvil_write(
    const void *restrict in,
    const size_t in_len,
    const double compression_level,
    const anvil_compression compression,
    const int64_t chunk_x,
    const int64_t chunk_z,
    anvil_file *file
) {
    anvil_assert(file != nullptr, return anvil_errno = ANVIL_INVALID_USAGE);
    anvil_assert(in_len == 0 || in != nullptr, return anvil_errno = ANVIL_INVALID_USAGE);
    anvil_assert(file->file != nullptr || file->size == 0, return anvil_errno = ANVIL_INVALID_USAGE);

    if (in_len == 0) {
        return chunk_realloc(
            file,
            nullptr,
            chunk_x,
            chunk_z,
            0
        );
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
        struct libdeflate_compressor *compressor = get_compressor((int)(compression_level * 12.0), file);
        if (compressor == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

        const size_t max_size = libdeflate_gzip_compress_bound(compressor, in_len);
        if (file->tmp_buffer_cap < max_size) {
            char *new = file->alloc->realloc(file->tmp_buffer, max_size);
            if (new == nullptr) {
                return ANVIL_ALLOC_FAILED;
            }

            file->tmp_buffer = new;
            file->tmp_buffer_cap = max_size;
        }

        chunk_size = libdeflate_gzip_compress(
            compressor,
            in,
            in_len,
            file->tmp_buffer,
            file->tmp_buffer_cap
        );

        chunk_data = file->tmp_buffer;
        break;
    }
    case ANVIL_COMPRESSION_ZLIB: {
        struct libdeflate_compressor *compressor = get_compressor((int)(compression_level * 12.0), file);
        if (compressor == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

        const size_t max_size = libdeflate_zlib_compress_bound(compressor, in_len);
        if (file->tmp_buffer_cap < max_size) {
            char *new = file->alloc->realloc(file->tmp_buffer, max_size);
            if (new == nullptr) {
                return ANVIL_ALLOC_FAILED;
            }

            file->tmp_buffer = new;
            file->tmp_buffer_cap = max_size;
        }

        chunk_size = libdeflate_zlib_compress(
            compressor,
            in,
            in_len,
            file->tmp_buffer,
            file->tmp_buffer_cap
        );

        chunk_data = file->tmp_buffer;
        break;
    }
    default: return ANVIL_UNSUPPORTED_COMPRESSION;
    }

    char header[5];

    header[0] = (char)((chunk_size + 1) >> (3 * 8));
    header[1] = (char)((chunk_size + 1) >> (2 * 8));
    header[2] = (char)((chunk_size + 1) >> (1 * 8));
    header[3] = (char)((chunk_size + 1) >> (0 * 8));
    header[4] = compression;

    if (chunk_size + 5 > UINT8_MAX * SECTOR_SIZE) {

        header[4] |= (char)(1<<7);

#ifdef CLOD_USE_POSIX

        const char *chunk_filename = get_chunk_filename(file, chunk_x, chunk_z);
        if (chunk_filename == nullptr) {
            return ANVIL_ALLOC_FAILED;
        }

        const int fd = openat(file->dir_fd, chunk_filename, O_CREAT | O_TRUNC);
        if (fd < 0) {
            return anvil_errno = anvil_errno_get(errno);
        }

        if (write(fd, header, 5) != 5) {
            const auto err = errno;
            close(fd);
            return anvil_errno = anvil_errno_get(err, errno = err);
        }

        if (write(fd, chunk_data, chunk_size) != chunk_size) {
            const auto err = errno;
            close(fd);
            return anvil_errno = anvil_errno_get(err, errno = err);
        }

        if (close(fd)) {
            return anvil_errno = anvil_errno_get(errno);
        }

#else
#error not implemented
#endif

        return ANVIL_OK;
    }

    char *cursor;
    const anvil_result res = chunk_realloc(
        file,
        &cursor,
        chunk_x,
        chunk_z,
        chunk_size + 5
    );
    if (res != ANVIL_OK) {
        return res;
    }

    memcpy(cursor, header, 5);
    memcpy(cursor + 5, chunk_data, chunk_size);
    return ANVIL_OK;
}

anvil_result anvil_close_file(anvil_file *file) {
    anvil_assert(file != nullptr, return anvil_errno = ANVIL_INVALID_USAGE);

    file->alloc->free(file->chunk_extension);
    if (file->tmp_string != nullptr)
        file->alloc->free(file->tmp_string);
    if (file->tmp_buffer != nullptr)
        file->alloc->free(file->tmp_buffer);
    if (file->libdeflate_compressor != nullptr)
        libdeflate_free_compressor(file->libdeflate_compressor);
    if (file->libdeflate_decompressor != nullptr)
        libdeflate_free_decompressor(file->libdeflate_decompressor);
    if (file->chunk_indices != nullptr)
        file->alloc->free(file->chunk_indices);

#ifdef CLOD_USE_POSIX

    if (
        file->file != nullptr &&
        munmap(file->file, file->size)
    ) {
        const auto err = errno;
        close(file->fd);
        return anvil_errno = anvil_errno_get(err, errno = err);
    }

    if (close(file->fd)) {
        return anvil_errno = anvil_errno_get(errno);
    }

#else
#error not implemented
#endif

    file->alloc->free(file);
    return ANVIL_OK;
}
