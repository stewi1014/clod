/**
 * @defgroup anvil anvil.h
 * @brief Anvil methods
 * @file anvil.h
 */

#ifndef ANVIL_H
#define ANVIL_H

#include <stdint.h>
#include <stddef.h>

/** Maximum number of coordinates. */
#define ANVIL_MAX_COORDINATES 10

/**
 * Maximum number of chunks in a single region file.
 * Approaching within a couple orders of magnitude of this means you've
 * missed the point of this kv storage system, and you need to get a different one.
 */
#define ANVIL_MAX_CHUNKS 1048576

//=======//
// Types //
//=======//

typedef struct anvil anvil;
typedef struct chunk chunk;
typedef struct anvil_opts anvil_opts;

typedef enum anvil_result {
    ANVIL_UNKNOWN       = -1,

    ANVIL_OK            = 0,
    ANVIL_MALFORMED     = 1,
    ANVIL_IO_ERROR      = 2,
    ANVIL_INVALID_USAGE = 3,
    ANVIL_ALLOC_FAILED  = 4,
    ANVIL_NOT_EXIST     = 5,
    ANVIL_NO_SPACE      = 6,
} anvil_result;

//=========//
// Methods //
//=========//

/**
 * Open an anvil directory.
 * @param path Path to the directory containing region files.
 * @param opts Configuration options.
 * @return Handle to the directory.
 */
anvil *anvil_open(const char *path, const anvil_opts *opts);

/**
 * Release resources associated with the handle.
 * @return Any error that occurred before or during the close.
 */
anvil_result anvil_close(anvil *a);

chunk *anvil_open_chunk(anvil *a, const int64_t *pos);
void anvil_close_chunk(anvil *a);

void *anvil_read(anvil *a, const int64_t *chunk_pos, int8_t *compression, size_t *size);
anvil_result anvil_read_done(anvil *a, void *);

void *anvil_write(anvil *a, const int64_t *chunk_pos, int8_t compression, size_t size);
anvil_result anvil_write_done(anvil *a, const void *);

//=========//
// Options //
//=========//

/**
 * Set all options to default.
 */
void anvil_opts_default(anvil_opts *o);

/**
 * Custom configuration options.
 */
struct anvil_opts {
    /**
     * File extension for region files.
     * Defaults to "mcr".
     */
    const char *region_extension;

    /**
     * File extension for chunk files.
     * Defaults to "mcc".
     */
    const char *chunk_extension;

    /**
     * Number of coordinates.
     * 0 < coord_count <= ANVIL_MAX_COORDINATES.
     * Defaults to 2.
     */
    int64_t coord_count;

    /**
     * The extent of chunks in the region file.
     * 0 < region_extent.
     * Defaults to 32.
     *
     * @note Recommended values:
     *  coord_count == 1  ? 1024
     *  coord_count == 2  ? 32
     *  coord_count == 3  ? 11
     *  coord_count == 4  ? 6
     *  coord_count == 5  ? 4
     *  coord_count == 6  ? 3
     *  coord_count == 7  ? 2
     *  coord_count == 8  ? 2
     *  coord_count == 9  ? 2
     *  coord_count == 10 ? 2
     *
     * @note Be careful with sizing this.
     *  The number of chunks (and hence header size) in each region file is
     *  region_extent raised to the power of coord_count.
     */
    int64_t region_extent;

    /**
     * Size of sections in the region file.
     * 0 < section_size.
     * Defaults to 4096.
     */
    size_t section_size;

    /**
     * @name Custom allocation methods.
     * Defaults to their stdlib equivalents.
     * @{
     */
    void *(*malloc)(size_t size);
    void *(*calloc)(size_t n, size_t size);
    void (*free)(void *ptr);
    void *(*realloc)(void *ptr, size_t size);
    /** @} */
};

//================//
// Error Handling //
//================//

/**
 * Get the error value.
 * @return Error result.
 */
anvil_result anvil_error();

/**
 * Get a detailed error message.
 * @return Error message.
 */
const char *anvil_strerror();

/**
 * Clear the error value and message.
 */
void anvil_clear_error();

/**
 * Set the method where messages are sent.
 * The default logger writes to stderr.
 */
void anvil_set_logger(void (*logger)(const char *msg));

#endif
