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
 * Approaching it means you missed the point of
 * this storage system a long time ago - get a new one.
 */
#define ANVIL_MAX_CHUNKS 1048576

//=======//
// Types //
//=======//

typedef struct anvil anvil;
typedef struct anvil_opts anvil_opts;
typedef enum anvil_result anvil_result;

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
 * This is not thread safe,
 * @return Any error that occurred before or during the close.
 */
anvil_result anvil_close(anvil *a);

/**
 * Read a chunk.
 *
 * @param[in] a Anvil instance handle.
 * @param[in] chunk_pos Chunk coordinates.
 * @param[out] compression Number in range [0,128).
 * @param[out] size Chunk data size.
 * @param[in] old_buffer (nullable) Buffer returned by a previous call to anvil_read.
 * If provided, it is equivalent to calling @link anvil_close_buffer @endlink on the old buffer,
 * but allows for the potential reuse of resources associated with the buffer.
 * The buffer is always closed, even on error, with the sole exception of when a or chunk_pos is null.
 * @return Buffer containing chunk data, or nullptr on error.
 * If the chunk is empty it returns a pointer to a single null byte.
 * The buffer must be closed after use.
 *
 * @note This method does not babysit bad software.
 * If you don't return the buffer, future operations may deadlock.
 * If you give it a garbage pointer in old_buffer behaviour is undefined.
 * If you write to the buffer permanent data corruption may occur.
 * In general, buffer misuse is not a recoverable error.
 */
const uint8_t *anvil_read(
    anvil *a,
    const int64_t *chunk_pos,
    int8_t *compression,
    size_t *size,
    const uint8_t *old_buffer
);

/**
 * Write a chunk.
 *
 * @param[in] a Anvil instance handle.
 * @param[in] chunk_pos Chunk coordinates.
 * @param[in] compression Number in range [0,128).
 * @param[in] size Size of chunk data.
 * @param[in] old_buffer (nullable) Buffer returned by a previous call to anvil_write.
 * If provided, it is equivalent to calling @link anvil_close_buffer @endlink on the old buffer,
 * but allows for the potential reuse of resources associated with the buffer.
 * The buffer is always closed, even on error, with the sole exception of when a or chunk_pos is null.
 * @return Buffer to write chunk data into.
 * If size is sero it returns a pointer to a single null byte.
 * The buffer must be closed after use.
 *
 * @note This method does not babysit bad software.
 * If you don't return the buffer, future operations will deadlock.
 * If you give it a garbage pointer in old_buffer behaviour is undefined.
 * If you write outside valid areas permanent data corruption may occur.
 * In general, buffer misuse is not a recoverable error.
 */
uint8_t *anvil_write(
    anvil *a,
    const int64_t *chunk_pos,
    int8_t compression,
    size_t size,
    const uint8_t *old_buffer
);

/**
 * Releases resources associated with a buffer.
 * This method may commit writes to the region file,
 * or perform other cleanup actions.
 *
 * @param[in] a Anvil instance handle.
 * @param[in] buffer The buffer.
 * @return Any error that occurred, or ANVIL_OK.
 */
anvil_result anvil_close_buffer(anvil *a, const uint8_t *buffer);

//================//
// Error Handling //
//================//

enum anvil_result {
    ANVIL_UNKNOWN       = -1,

    ANVIL_OK            = 0,
    ANVIL_MALFORMED     = 1,
    ANVIL_IO_ERROR      = 2,
    ANVIL_INVALID_USAGE = 3,
    ANVIL_ALLOC_FAILED  = 4,
    ANVIL_NOT_EXIST     = 5,
    ANVIL_NO_SPACE      = 6,
};

/**
 * Get the error value.
 * @return Error result.
 */
anvil_result anvil_error();

/**
 * Get a detailed error message.
 * The message is valid until the next
 * @return Error message.
 */
const char *anvil_strerror();

/**
 * Clear the error value.
 */
void anvil_clear_error();

/**
 * Set the method where messages are sent.
 * The default logger writes to stderr.
 */
void anvil_set_logger(void (*logger)(const char *msg));

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
     * Defaults to 2.
     */
    int64_t coord_count;

    /**
     * The extent of chunks in the region file.
     * Defaults to 32.
     *
     * @note Recommended values:
     *  coord_count == 0  ? 1
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
    void *(*realloc)(void *restrict ptr, size_t size);
    /** @} */
};

#endif
