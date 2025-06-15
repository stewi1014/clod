#include <libdeflate.h>
#include <string.h>

#include "error.h"
#include "file.h"

#include <fcntl.h>

#include "filename.h"

#ifdef CLOD_USE_POSIX
#include <unistd.h>
#else
#error not implemented
#endif

#define MAGIC "libclod region format v1"
#define MAGIC_SIZE 32

#define TABLE_SIZE(layout) (chunk_count(layout) * 4)
#define MINECRAFT_HEADER_SIZE(layout) (TABLE_SIZE(layout) * 2)
#define LIBCLOD_HEADER_SIZE(layout) (TABLE_SIZE(layout) * 2 + MAGIC_SIZE + TABLE_SIZE(layout) + 4 + 4 + 1)

#define SECTION_OFFSET(pos) (pos >> 8)
#define SECTION_COUNT(pos) (pos & 0xFF)

anvil_file *anvil_file_new(
    const char *filename, //only used for error messages.
    const coord_layout_t layout,
    const int64_t *region,
    const size_t section_size,
    const anvil_allocator alloc,

    #ifdef CLOD_USE_POSIX
    const int fd
    #else
    #error not implemented
    #endif
) {
    const size_t filename_size = strlen(filename) + 1;

    anvil_file *file = alloc.malloc(
        sizeof(anvil_file) +
        filename_size +
        layout.count * sizeof(int64_t)
    );
    if (!file) {
        anvil_set_result(ANVIL_ALLOC_FAILED, nullptr);
        return nullptr;
    }

    file->header_buffer = alloc.malloc(LIBCLOD_HEADER_SIZE(layout));
    if (!file->header_buffer) {
        anvil_set_result(ANVIL_ALLOC_FAILED, nullptr);
        alloc.free(file);
        return nullptr;
    }

    file->filename = file->__ext;
    file->region = (int64_t*)(file->__ext + filename_size);
    file->layout = layout;
    file->section_size = section_size;
    file->alloc = alloc;

    coord_copy(file->region, region, layout);

    #ifdef CLOD_USE_POSIX
    file->fd = fd;

    /** This is not pointless. It (hopefully) moves file errors to open time instead of read/write time. */
    if (read(fd, file->header_buffer, 0) != 0) {
        anvil_set_errno(errno, "Opening ", file->filename);
        alloc.free(file->header_buffer);
        alloc.free(file);
        return nullptr;
    }

    #else
    #error not defined
    #endif

    return file;
}

void anvil_close_file(anvil_file *file) {
    #ifdef CLOD_USE_POSIX
    if (close(file->fd) && anvil_error() == ANVIL_OK) {
        anvil_set_errno(errno, "Closing ", file->filename);
    }
    #else
    #error not implemented
    #endif

    file->alloc.free(file->header_buffer);
    file->alloc.free(file);
}

typedef struct {
    uint32_t *active;
    uint32_t *mtime;
    uint32_t *inactive;
} header_t;

static anvil_result read_header(
    anvil_file *file,
    header_t *header
) {
    header->active = nullptr;
    header->mtime = nullptr;
    header->inactive = nullptr;

    #ifdef CLOD_USE_POSIX
    const off64_t file_size = lseek(file->fd, 0, SEEK_END);
    if (file_size < 0) {
        return anvil_set_errno(errno, "Getting file size %s", file->filename);
    }

    size_t header_size = MINECRAFT_HEADER_SIZE(file->layout);
    if (file_size < header_size) {
        return ANVIL_OK;
    }

    if (file_size >= header_size + MAGIC_SIZE) {
        header_size += MAGIC_SIZE;
    }

    if (lseek(file->fd, 0, SEEK_SET) < 0) {
        anvil_set_errno(errno, "Seeking %s", file->filename);
    }

    size_t total = 0;
    while (total < header_size) {
        const ssize_t r = read(file->fd, file->header_buffer, header_size - total);
        if (r < 0) {
            return anvil_set_errno(errno, "Reading %s", file->filename);
        }
        total += r;

        if (
            total == header_size &&
            strncmp((char*)file->header_buffer + total - MAGIC_SIZE, MAGIC, MAGIC_SIZE) == 0
        ) {
            // We've got the magic!
            header_size = LIBCLOD_HEADER_SIZE(file->layout);
        }
    }

    #else
    #error not implemented
    #endif

    const size_t table_size = TABLE_SIZE(file->layout);

    header->active = (uint32_t*)file->header_buffer;
    header->mtime = (uint32_t*)(file->header_buffer + table_size);

    if (header_size == LIBCLOD_HEADER_SIZE(file->layout)) {
        header->inactive = (uint32_t*)(file->header_buffer + table_size * 2 + MAGIC_SIZE);
        // Magic is in the air
        const uint8_t *cursor = file->header_buffer + table_size * 2 + MAGIC_SIZE + table_size;
        uint32_t active_checksum =
            cursor[0] << 24 |
            cursor[1] << 16 |
            cursor[2] << 8 |
            cursor[3];
        uint32_t inactive_checksum =
            cursor[4] << 24 |
            cursor[5] << 16 |
            cursor[6] << 8 |
            cursor[7];
        const bool second_is_active = cursor[8] & 1 > 0;

        if (second_is_active) {
            uint32_t *tmp = header->active;
            const uint32_t tmp2 = active_checksum;
            header->active = header->inactive;
            active_checksum = inactive_checksum;
            header->inactive = tmp;
            inactive_checksum = tmp2;
        }

        const uint32_t actual_active_checksum =
            libdeflate_crc32(0, header->active, table_size);
        const uint32_t actual_inactive_checksum =
            libdeflate_crc32(0, header->inactive, table_size);

        if (actual_active_checksum != active_checksum) {
            if (actual_inactive_checksum != inactive_checksum) {
                anvil_set_result(ANVIL_MALFORMED, "Both headers are corrupt in %s. You're on your own.", file->filename);
            }

            memcpy(header->active, header->inactive, table_size);
            active_checksum = inactive_checksum;
        } else {

        }

    } else {
        header->inactive = (uint32_t*)(file->header_buffer + table_size * 2 + MAGIC_SIZE);
        memcpy(header->inactive, header->active, table_size);

        const uint32_t active_checksum = libdeflate_crc32(0, header->active, table_size);
        const uint32_t inactive_checksum = active_checksum;
    }
}

static bool write_header(anvil_file *file, header_t *header) {

}

size_t chunk_size(
    anvil_file *file,
    int64_t *chunk_coords
);

time_t anvil_file_mtime(
    anvil_file *file,
    int64_t *chunk_coords
) {
    return 0;
}

anvil_result read_chunk(
    uint8_t *restrict buffer,
    size_t buffer_size,
    int64_t *chunk_coords,
    anvil_file *file
) {
    return ANVIL_OK;
}

anvil_result write_chunk(
    anvil_file *file,
    int64_t *chunk_coords,
    uint8_t *restrict buffer,
    size_t buffer_size
) {
    return ANVIL_OK;
}
