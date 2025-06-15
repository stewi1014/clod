#include <string.h>
#include <anvil.h>
#include "anvil.h"
#include "vec.h"

#ifdef CLOD_POSIX
#include <fcntl.h>
#include <sys/stat.h>
#else
#error not implemented
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
        anvil_assert(path != nullptr) ||
        anvil_assert(opts->region_extension) ||
        anvil_assert(opts->chunk_extension) ||
        anvil_assert(opts->coord_count > 0) ||
        anvil_assert(opts->coord_count <= ANVIL_MAX_COORDINATES) ||
        anvil_assert(opts->region_extent > 0) ||
        anvil_assert(opts->section_size > 0) ||
        anvil_assert(powl(opts->region_extent, opts->coord_count) <= ANVIL_MAX_CHUNKS) ||
        anvil_assert(opts->malloc != nullptr) ||
        anvil_assert(opts->calloc != nullptr) ||
        anvil_assert(opts->free != nullptr) ||
        anvil_assert(opts->realloc != nullptr)
    ) {
        return nullptr;
    }

    anvil *a = opts->malloc(
        sizeof(anvil) +
        strlen(path) + 1
    );

    if (!a) {
        set_result(ANVIL_ALLOC_FAILED, nullptr);
        return nullptr;
    }

    a->path = a->__ext;
    strcpy(a->__ext, path);

    mutex_init(&a->mutex);
    a->region_files = nullptr;
    a->buffers_len = 0;
    a->buffers_cap = 0;
    a->buffers = nullptr;
    a->opts = *opts;

    #ifdef CLOD_POSIX

    a->dir_fd = open(a->path, O_RDONLY | O_DIRECTORY);
    if (a->dir_fd < 0) {
        set_errno("Opening region directory %s", a->path);
        mutex_destroy(&a->mutex);
        opts->free(a);
        return nullptr;
    }

    #else
    #error not implemented
    #endif

    return a;
}

/*

region_file *clopen_region_file(
    anvil *const a,
    region_file *old_rfile,
    int64_t const *const chunk_pos
) {
    int64_t region_pos[a->opts.coord_count];
    vec_div(a->opts.coord_count, region_pos, chunk_pos, a->opts.region_extent);

    if (old_rfile && vec_equal(a->opts.coord_count, old_rfile->region_pos, region_pos)) {
        if (old_rfile->ref_count == 0) {
            LIST_INSERT_HEAD(&a->region_files, old_rfile, node);
        }
        old_rfile->ref_count++;
        return old_rfile;
    }

    region_file *new_rfile;
    LIST_FOREACH(new_rfile, &a->region_files, node) {
        if (vec_equal(a->opts.coord_count, new_rfile->region_pos, region_pos)) {
            if (old_rfile && old_rfile->ref_count == 0) {
                if (region_file_close(a, old_rfile)) {
                    return nullptr;
                }
            }
        }
    }
}

region_file *get_region_file(
    const anvil *const a,
    int64_t const *const chunk_pos
) {
    int64_t region_pos[a->opts.coord_count];
    vec_div(a->opts.coord_count, region_pos, chunk_pos, a->opts.region_extent);

    region_file *rfile;
    LIST_FOREACH(rfile, &a->region_files, node) {
        if (vec_equal(a->opts.coord_count, rfile->region_pos, region_pos)) {
            return rfile;
        }
    }

    return nullptr;
}

chunk_file *get_chunk_file(
    const anvil *const a,
    const region_file *const rfile,
    int64_t const *const chunk_pos
) {
    chunk_file *cfile;
    LIST_FOREACH(cfile, &rfile->chunk_files, node) {
        if (vec_equal(a->opts.coord_count, cfile->chunk_pos, chunk_pos)) {
            return cfile;
        }
    }

    return nullptr;
}

/**
 * Read a chunk.
 *
 * @param[in] a Anvil instance handle.
 * @param[in] chunk_pos Chunk coordinates.
 * @param[out] compression Number in range [0,128).
 * @param[out] size Chunk data size.
 * @param[in] old_buffer (nullable) Pointer to any byte, or the byte following, the buffer returned by a previous call to anvil_read.
 * If provided, it is equivalent to calling @link anvil_close_buffer @endlink on the old buffer,
 * but allows for the potential reuse of resources associated with the buffer.
 * The buffer is always closed, even on error, with the sole exception of misuse through a or chunk_pos being null.
 * @return Buffer containing chunk data, or nullptr on error.
 * If the chunk is empty it returns a pointer to a single null byte.
 * The buffer must be closed after use.
 *
 * @note This method does not babysit bad software.
 * If you don't return the buffer, future operations will deadlock.
 * If you give it a garbage pointer in old_buffer behaviour is undefined.
 * If you write to the buffer permanent data corruption may occur.
 * In general, buffer misuse is not a recoverable error.
 *
const uint8_t *anvil_read(
    anvil *const a,
    int64_t const *const chunk_pos,
    int8_t *const compression,
    size_t *const size,
    uint8_t const *const old_buffer
) {
    if (
        anvil_assert(a != nullptr) ||
        anvil_assert(chunk_pos != nullptr)
    ) {
        return nullptr;
    }

    int64_t region_pos[a->opts.coord_count];
    vec_div(a->opts.coord_count, region_pos, chunk_pos, a->opts.region_extent);

    if (!old_buffer) rwmutex_lock(&a->io_mutex);
    mutex_lock(&a->mutex);

    region_file *old_rfile = nullptr;
    chunk_file *old_cfile = nullptr;
    if (old_buffer && !pop_buffer(a, old_buffer, &old_rfile, &old_cfile)) {
        set_result(ANVIL_INVALID_USAGE, "Invalid old buffer value");
        mutex_unlock(&a->mutex);
        return nullptr;
    }

    region_file *rfile = get_region_file(a, chunk_pos);
    if (!rfile) {
        rfile = region_file_open(a, old_rfile, region_pos);
        if (!rfile) {
            if (old_cfile) preserve_error(chunk_file_close(a, old_cfile));
            mutex_unlock(&a->mutex);
            rwmutex_rdunlock(&a->io_mutex);
            return nullptr;
        }
    } else if (old_rfile) {
        const anvil_result res = region_file_close(a, old_rfile);
        if (res != ANVIL_OK) {
            if (old_cfile) preserve_error(chunk_file_close(a, old_cfile));
            mutex_unlock(&a->mutex);
            rwmutex_rdunlock(&a->io_mutex);
            return nullptr;
        }
    }

    size_t chunk_size;
    uint8_t *chunk_data = region_file_read(a, rfile, chunk_pos, &chunk_size);
    if (!chunk_data) {
        if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
        if (old_cfile) preserve_error(chunk_file_close(a, old_cfile));
        mutex_unlock(&a->mutex);
        rwmutex_rdunlock(&a->io_mutex);
        return nullptr;
    }

    if (chunk_size == 0) {
        if (old_cfile) {
            const anvil_result res = chunk_file_close(a, old_cfile);
            if (res != ANVIL_OK) {
                if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
                mutex_unlock(&a->mutex);
                rwmutex_rdunlock(&a->io_mutex);
                return nullptr;
            }
        }

        if (rfile->ref_count == 0) {
            LIST_INSERT_HEAD(&a->region_files, rfile, node);
        }
        rfile->ref_count++;
        mutex_unlock(&a->mutex);
        if (size) *size = 0;
        return rfile->data;
    }

    if (chunk_size < 5 || chunk_size < read_be32(chunk_data)) {
        if (rfile->ref_count == 0) region_file_close(a, rfile);
        if (old_cfile) chunk_file_close(a, old_cfile);
        set_result(ANVIL_MALFORMED, "Invalid chunk size");
        mutex_unlock(&a->mutex);
        rwmutex_rdunlock(&a->io_mutex);
        return nullptr;
    }

    if (CHUNK_SEPARATE(chunk_data)) {
        chunk_file *cfile = get_chunk_file(a, rfile, chunk_pos);
        if (!cfile) {
            cfile = chunk_file_open(a, old_cfile, chunk_pos);
            if (!cfile) {
                if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
                mutex_unlock(&a->mutex);
                rwmutex_rdunlock(&a->io_mutex);
                return nullptr;
            }
        } else if (old_cfile) {
            const anvil_result res = chunk_file_close(a, old_cfile);
            if (res != ANVIL_OK) {
                if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
                mutex_unlock(&a->mutex);
                rwmutex_rdunlock(&a->io_mutex);
                return nullptr;
            }
        }

        chunk_data = chunk_file_read(a, cfile, &chunk_size);
        if (!chunk_data) {
            if (cfile->ref_count == 0) preserve_error(chunk_file_close(a, cfile));
            if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
            mutex_unlock(&a->mutex);
            rwmutex_rdunlock(&a->io_mutex);
            return nullptr;
        }

        if (cfile->ref_count == 0) {
            LIST_INSERT_HEAD(&rfile->chunk_files, cfile, node);
        }
        cfile->ref_count++;
        if (rfile->ref_count == 0) {
            LIST_INSERT_HEAD(&a->region_files, rfile, node);
        }
        rfile->ref_count++;
        mutex_unlock(&a->mutex);
        if (size) *size = chunk_size - 5;
        if (compression) *compression = CHUNK_COMPRESSION(chunk_data);
        return chunk_data;
    }

    if (old_cfile) {
        const anvil_result res = chunk_file_close(a, old_cfile);
        if (res != ANVIL_OK) {
            if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
            mutex_unlock(&a->mutex);
            rwmutex_rdunlock(&a->io_mutex);
            return nullptr;
        }
    }

    if (rfile->ref_count == 0) {
        LIST_INSERT_HEAD(&rfile->chunk_files, rfile, node);
    }
    rfile->ref_count++;
    if (size) *size = read_be32(chunk_data) - 5;
    if (compression) *compression = CHUNK_COMPRESSION(chunk_data);
    mutex_unlock(&a->mutex);
    return chunk_data + 5;
}

/**
 * Write a chunk.
 *
 * @param[in] a Anvil instance handle.
 * @param[in] chunk_pos Chunk coordinates.
 * @param[in] compression Number in range [0,128).
 * @param[in] size Size of chunk data.
 * @param[in] old_buffer (nullable) Pointer to any byte, or the byte following, the buffer returned by a previous call to anvil_write.
 * If provided, it is equivalent to calling @link anvil_close_buffer @endlink on the old buffer,
 * but allows for the potential reuse of resources associated with the buffer.
 * The buffer is always closed, even on error, with the sole exception of misuse through a or chunk_pos being null.
 * @return Buffer to write chunk data into.
 * If size is sero it returns a pointer to a single null byte.
 * The buffer must be closed after use.
 *
 * @note This method does not babysit bad software.
 * If you don't return the buffer, future operations will deadlock.
 * If you give it a garbage pointer in old_buffer behaviour is undefined.
 * If you write outside valid areas permanent data corruption may occur.
 * In general, buffer misuse is not a recoverable error.
 *
uint8_t *anvil_write(
    anvil *const a,
    int64_t const *const chunk_pos,
    int8_t const compression,
    size_t const size,
    uint8_t const *const old_buffer
) {
    if (
        anvil_assert(a != nullptr) ||
        anvil_assert(chunk_pos != nullptr)
    ) {
        return nullptr;
    }

    int64_t region_pos[a->opts.coord_count];
    vec_div(a->opts.coord_count, region_pos, chunk_pos, a->opts.region_extent);

    if (!old_buffer) rwmutex_lock(&a->io_mutex);
    mutex_lock(&a->mutex);

    region_file *old_rfile = nullptr;
    chunk_file *old_cfile = nullptr;
    if (old_buffer && !pop_buffer(a, old_buffer, &old_rfile, &old_cfile)) {
        set_result(ANVIL_INVALID_USAGE, "Invalid old buffer value");
        mutex_unlock(&a->mutex);
        return nullptr;
    }

    region_file *rfile = get_region_file(a, chunk_pos);
    if (!rfile) {
        rfile = region_file_open(a, old_rfile, region_pos);
        if (!rfile) {
            if (old_cfile) preserve_error(chunk_file_close(a, old_cfile));
            mutex_unlock(&a->mutex);
            rwmutex_rdunlock(&a->io_mutex);
            return nullptr;
        }
    } else if (old_rfile) {
        const anvil_result res = region_file_close(a, old_rfile);
        if (res != ANVIL_OK) {
            if (old_cfile) preserve_error(chunk_file_close(a, old_cfile));
            mutex_unlock(&a->mutex);
            rwmutex_rdunlock(&a->io_mutex);
            return nullptr;
        }
    }

    size_t chunk_size;
    uint8_t *chunk_data = region_file_write(a, rfile, chunk_pos, &chunk_size);
    if (!chunk_data) {
        if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
        if (old_cfile) preserve_error(chunk_file_close(a, old_cfile));
        mutex_unlock(&a->mutex);
        rwmutex_rdunlock(&a->io_mutex);
        return nullptr;
    }

    if (chunk_size == 0) {
        if (old_cfile) {
            const anvil_result res = chunk_file_close(a, old_cfile);
            if (res != ANVIL_OK) {
                if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
                mutex_unlock(&a->mutex);
                rwmutex_rdunlock(&a->io_mutex);
                return nullptr;
            }
        }

        if (rfile->ref_count == 0) {
            LIST_INSERT_HEAD(&a->region_files, rfile, node);
        }
        rfile->ref_count++;
        mutex_unlock(&a->mutex);
        if (size) *size = 0;
        return rfile->data;
    }

    if (chunk_size < 5 || chunk_size < read_be32(chunk_data)) {
        if (rfile->ref_count == 0) region_file_close(a, rfile);
        if (old_cfile) chunk_file_close(a, old_cfile);
        set_result(ANVIL_MALFORMED, "Invalid chunk size");
        mutex_unlock(&a->mutex);
        rwmutex_rdunlock(&a->io_mutex);
        return nullptr;
    }

    if (CHUNK_SEPARATE(chunk_data)) {
        chunk_file *cfile = get_chunk_file(a, rfile, chunk_pos);
        if (!cfile) {
            cfile = chunk_file_open(a, old_cfile, chunk_pos);
            if (!cfile) {
                if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
                mutex_unlock(&a->mutex);
                rwmutex_rdunlock(&a->io_mutex);
                return nullptr;
            }
        } else if (old_cfile) {
            const anvil_result res = chunk_file_close(a, old_cfile);
            if (res != ANVIL_OK) {
                if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
                mutex_unlock(&a->mutex);
                rwmutex_rdunlock(&a->io_mutex);
                return nullptr;
            }
        }

        chunk_data = chunk_file_read(a, cfile, &chunk_size);
        if (!chunk_data) {
            if (cfile->ref_count == 0) preserve_error(chunk_file_close(a, cfile));
            if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
            mutex_unlock(&a->mutex);
            rwmutex_rdunlock(&a->io_mutex);
            return nullptr;
        }

        if (cfile->ref_count == 0) {
            LIST_INSERT_HEAD(&rfile->chunk_files, cfile, node);
        }
        cfile->ref_count++;
        if (rfile->ref_count == 0) {
            LIST_INSERT_HEAD(&a->region_files, rfile, node);
        }
        rfile->ref_count++;
        mutex_unlock(&a->mutex);
        if (size) *size = chunk_size - 5;
        if (compression) *compression = CHUNK_COMPRESSION(chunk_data);
        return chunk_data;
    }

    if (old_cfile) {
        const anvil_result res = chunk_file_close(a, old_cfile);
        if (res != ANVIL_OK) {
            if (rfile->ref_count == 0) preserve_error(region_file_close(a, rfile));
            mutex_unlock(&a->mutex);
            rwmutex_rdunlock(&a->io_mutex);
            return nullptr;
        }
    }

    if (rfile->ref_count == 0) {
        LIST_INSERT_HEAD(&rfile->chunk_files, rfile, node);
    }
    rfile->ref_count++;
    if (size) *size = read_be32(chunk_data) - 5;
    if (compression) *compression = CHUNK_COMPRESSION(chunk_data);
    mutex_unlock(&a->mutex);
    return chunk_data + 5;
}

/**
 * Releases resources associated with a buffer.
 * This method may commit writes to the region file,
 * or perform other cleanup actions.
 *
 * @param[in] a Anvil instance handle.
 * @param[in] buffer The buffer.
 * @return Any error that occurred, or ANVIL_OK.
 *
anvil_result anvil_close_buffer(anvil *a, const uint8_t *buffer) {
    if (anvil_assert(a != nullptr) || anvil_assert(buffer != nullptr)) {
        return ANVIL_INVALID_USAGE;
    }

    mutex_lock(&a->mutex);
    region_file *rfile;
    chunk_file *cfile;
    if (!pop_buffer(a, buffer, &rfile, &cfile)) {
        set_result(ANVIL_INVALID_USAGE, "Invalid buffer value");
        mutex_unlock(&a->mutex);
        return ANVIL_INVALID_USAGE;
    }

    anvil_result r_res, c_res;

    if (cfile) {
        c_res = chunk_file_close(a, cfile);
    }

    if (rfile) {
        r_res = region_file_close(a, rfile);
    }

    if (r_res) return r_res;
    return c_res;
}

anvil_result anvil_close(anvil *a) {
    #ifdef CLOD_POSIX
    if (close(a->dir_fd) < 0) {
        set_errno("Closing region directory %s", a->path);
    }
    #else
    #error not implemented
    #endif

    rwmutex_unlock(&a->io_mutex);
    mutex_destroy(&a->mutex);
    a->opts.free(a);
    return anvil_error();
}
*/