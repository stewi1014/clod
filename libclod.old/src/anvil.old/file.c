#include <libdeflate.h>

#include "file.h"

#include <assert.h>
#include <string.h>

#include "coord.h"
#include "error.h"
#include "filename.h"

#include <nghttp3/nghttp3.h>

#ifdef CLOD_POSIX
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#else
#error not implemented
#endif

#define crc32(data, size) libdeflate_crc32(0, data, size)



#define file_name(file)     anvil_create_filename("r",      file->opts.region_extension, file->region, file->opts.layout)
#define sh_name(file)       anvil_create_filename("sh",     file->opts.region_extension, file->region, file->opts.layout)
#define new_sh_name(file)   anvil_create_filename("new_sh", file->opts.region_extension, file->region, file->opts.layout)
#define old_sh_name(file)   anvil_create_filename("old_sh", file->opts.region_extension, file->region, file->opts.layout)

#define TABLE_SIZE(layout) (chunk_count(layout) * 4)

#define POS_TABLE_OFF(layout)           (TABLE_SIZE(layout) * 0)
#define MTIME_TABLE_OFF(layout)         (TABLE_SIZE(layout) * 1)
    #define HEADER_SIZE(layout)         (TABLE_SIZE(layout) * 2)

#define FILE_CHECKSUM_TABLE_OFF(layout) (TABLE_SIZE(layout) * 2)
#define SH_CHECKSUM_TABLE_OFF(layout)   (TABLE_SIZE(layout) * 3)
#define CHECKSUM_OFF(layout)            (TABLE_SIZE(layout) * 4)
    #define SH_SIZE(layout)             (TABLE_SIZE(layout) * 4 + 4)

static uint32_t read_be32(const uint8_t *d) {
    return d[0] << (3 * 8) | d[1] << (2 * 8) | d[2] << (1 * 8) | d[3] << (0 * 8);
}

static void write_be32(uint8_t *d, const uint32_t v) {
    d[0] = v >> (3 * 8); d[1] = v >> (2 * 8); d[2] = v >> (1 * 8); d[3] = v >> (0 * 8);
}

static
struct {
    size_t offset;
    size_t size;
} get_pos(
    const uint8_t *restrict data,
    const size_t index
) {return {
        data[index * 4] << 16 | data[index * 4 + 1] << 8 | data[index * 4 + 2],
        data[index * 4 + 3]
};}

static
uint32_t get_uint32(
    const uint8_t *restrict data,
    const size_t index
) {
    return data[index * 4] << 24 |
        data[index * 4 + 1] << 16 |
        data[index * 4 + 2] << 8 |
        data[index * 4 + 3];
}

anvil_file *anvil_file_open(
    const int64_t *region,
    const anvil_opts opts,

    #ifdef CLOD_POSIX
    const int dir_fd
    #else
    #error not implemented
    #endif
) {
    anvil_file *file = opts.alloc->malloc(
        sizeof(*file) +
        opts.layout.count * sizeof(int64_t) +
        SH_SIZE(opts.layout)
    );
    if (!file) {
        anvil_set_result(ANVIL_ALLOC_FAILED, nullptr);
        return nullptr;
    }

    file->size = 0;
    file->opts = opts;
    file->region = file->__ext;
    file->sh = file->__ext + opts.layout.count * sizeof(int64_t);

    coord_copy(file->region, region, opts.layout);

#ifdef CLOD_POSIX
    struct stat st;

    file->file = nullptr;
    file->sh = nullptr;
    file->dir_fd = dir_fd;
    file->file_fd = -1;
    int sh = -1;

    bool read_only;
    file->file_fd = openat(dir_fd, file_name(file), O_RDWR | O_CREAT);
    if (file->file_fd < 0) {
        if (errno != EACCES) {
            anvil_set_errno(errno, "Opening region file %s", file_name(file));
            goto error;
        }

        file->file_fd = openat(dir_fd, file_name(file), O_RDONLY);
        if (file->file_fd < 0) {
            anvil_set_errno(EACCES, "Opening region file %s", file_name(file));
            goto error;
        }
        read_only = true;
    } else {
        read_only = false;
    }

    if (
        read(file->file_fd, file, 0) < 0 ||
        (!read_only && write(file->file_fd, file, 0) < 0)
    ) {
        anvil_set_errno(errno, "Testing region file %s", file_name(file));
        goto error;
    }

    if (fstat(file->file_fd, &st) < 0) {
        anvil_set_errno(errno, "Failed stating region file %s", file_name(file));
        goto error;
    }
    file->size = st.st_size;
    if (st.st_size == 0) {
        return file;
    }
    if (st.st_size < HEADER_SIZE(opts.layout)) {
        anvil_set_result(ANVIL_MALFORMED, "Region file %s is too small", file_name(file));
        goto error;
    }

    file->file = mmap(
        nullptr,
        file->size,
        PROT_READ | PROT_WRITE,
        read_only ? MAP_PRIVATE : MAP_SHARED,
        file->file_fd,
        0
    );
    if (file->file == MAP_FAILED) {
        file->file = nullptr;
        anvil_set_errno(errno, "Mapping region file %s", file_name(file));
        goto error;
    }

    if (madvise(file->file, HEADER_SIZE(opts.layout), MADV_WILLNEED) < 0) {
        anvil_set_errno(errno, "Paging in header from %s", file_name(file));
        goto error;
    }

    sh = openat(dir_fd, sh_name(file), O_RDONLY);
    if (sh < 0) {
        if (errno != ENOENT) {
            anvil_set_errno(errno, "Opening shadow header %s", sh_name(file));
            goto error;
        }

        sh = openat(dir_fd, old_sh_name(file), O_RDONLY);
        if (sh < 0) {
            if (errno != ENOENT) {
                anvil_set_errno(errno, "Opening shadow header %s", old_sh_name(file));
                goto error;
            }

            // No shadow header. Make one.
            memcpy(file->sh, file->file, HEADER_SIZE(opts.layout));
            memset(file->sh + HEADER_SIZE(opts.layout), 0, SH_SIZE(opts.layout) - HEADER_SIZE(opts.layout));
            write_be32(file->sh + CHECKSUM_OFF(opts.layout), crc32(file->file, HEADER_SIZE(opts.layout)));
            return file;
        }
    }

    size_t n = 0;
    while (n < SH_SIZE(opts.layout)) {
        const ssize_t c = read(sh, file->sh + n, SH_SIZE(opts.layout) - n);
        if (c < 0) {
            anvil_set_errno(errno, "Reading shadow header %s", sh_name(file));
            goto error;
        }

        n += c;
    }

    if (close(sh) < 0) {
        anvil_set_errno(errno, "Closing shadow header %s", sh_name(file));
        goto error;
    }

    if (read_be32(file->sh + CHECKSUM_OFF(opts.layout)) != crc32(file->file, HEADER_SIZE(opts.layout))) {
        // region file header is corrupted.
        // restore from shadow header.
        memcpy(file->file, file->sh, HEADER_SIZE(opts.layout));
        write_be32(file->sh + CHECKSUM_OFF(opts.layout), crc32(file->file, HEADER_SIZE(opts.layout)));
        memcpy(file->sh + FILE_CHECKSUM_TABLE_OFF(opts.layout), file->sh + SH_CHECKSUM_TABLE_OFF(opts.layout), TABLE_SIZE(opts.layout));
    }

    return file;
error:
    if (file->sh != nullptr) munmap(file->sh, SH_SIZE(opts.layout));
    if (sh >= 0) close(sh);
    if (file->file != nullptr) munmap(file->file, file->size);
    if (file->file_fd >= 0) close(file->file_fd);
    opts.alloc->free(file);
    return nullptr;

#else
    #error not implemented - sorry, this one is a bit of a doozie.
#endif
}

anvil_result anvil_close_file(anvil_file *file) {
    anvil_result res = ANVIL_OK;

    #ifdef CLOD_POSIX
    if (
        file->sh != nullptr &&
        munmap(file->sh, HEADER_SIZE(file->opts.layout)) < 0
    ) {
        res = anvil_set_errno(errno, "Closing shadow header %s", file_name(file));
    }

    if (
        file->file != nullptr &&
        munmap(file->file, file->size) < 0
    ) {
        res = anvil_set_errno(errno, "Closing region file %s", file_name(file));
    }

    if (
        file->file_fd >= 0 &&
        close(file->file_fd) < 0
    ) {
        res = anvil_set_errno(errno, "Closing region file %s", file_name(file));
    }

    #else
    #error not implemented
    #endif

    return res;
}

const uint8_t *read_chunk(
    anvil_file *file,
    const size_t index
) {
    if (anvil_assert(index < chunk_count(file->opts.layout))) {
        return nullptr;
    }

    const auto pos = get_pos(file->file, index);
    if ((pos.offset + pos.size) * file->opts.section_size > file->size) {

    }

    if (file->sh) {
        if (!file->checksum_valid) {
            file->checksum = crc32(file->file, HEADER_SIZE(file->opts.layout));
            if (file->checksum != )
        }


        const auto pos = get_pos(file->sh, index);

    }

    const auto pos = get_pos(file->file, index);
    if (file->sh != nullptr) {
        if (crc32())
    }

    if ((pos.offset + pos.size) * file->opts.section_size > file->size) {
        anvil_set_result
    }
}
