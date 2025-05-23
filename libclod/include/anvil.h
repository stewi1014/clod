#pragma once
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/**
 * @defgroup anvil anvil.h
 *
 * @example open_world.c
 * @example iterate_regions.c
 *
 * @{
 * @file anvil.h
 * This header defines methods for read and writing to minecraft worlds.
 * @link anvil_world_open @endlink is the primary entrypoint.
 */

/**
 * Message handler. It writes to stderr by default,
 * and I would recommend replacing it in all environments except
 * pure C that only interacts with stderr using stdlib methods.
 *
 * Messages are sparingly used, and only emitted due to user error.
 * If this library is used correctly no messages should be emitted.
 *
 * The return value is never used, and exists only to confirm to the printf function signature.
 * Can be set to null to disable messages.
 */
extern int (*anvil_message_printf)(const char *format, ...);

/**
 * custom memory allocator.
 *
 * methods must follow the behaviour of malloc, free, calloc and realloc.
 * [malloc manpage](https://man7.org/linux/man-pages/man3/malloc.3.html)
 */
typedef struct {
    void *(*malloc)(size_t size);
    void *(*calloc)(size_t n, size_t size);
    void (*free)(void *ptr);
    void *(*realloc)(void *restrict ptr, size_t size);
} anvil_allocator;

/**
 * Used to describe the result of operations
 *
 *
 */
typedef enum anvil_result_e {
    /** Operation was successful. */
    ANVIL_OK                      = 1,
    /** The given buffer is not large enough to hold the requested data. */
    ANVIL_TOO_SMALL               = 2,
    /** Input data is malformed. */
    ANVIL_MALFORMED               = 3,
    /** An IO error occurred. */
    ANVIL_ERROR_IO                = 4,
    /** Method was used incorrectly or given invalid arguments. */
    ANVIL_INVALID_USAGE           = 5,
    /** Memory or other resource allocation failed. */
    ANVIL_ALLOC_FAILED            = 6,
    /** The resource is locked. */
    ANVIL_LOCKED                  = 7,
    /** The resource does not exist. */
    ANVIL_NOT_EXIST               = 8,
    /** The multipart operation is finished. no more calls should be made. */
    ANVIL_DONE                    = 9,
    /** Compression regime is not supported. */
    ANVIL_UNSUPPORTED_COMPRESSION = 10,
    /** Unable to interpret errno after an error. */
    ANVIL_ERRNO                   = 11,
    /** The disk no space remaining. */
    ANVIL_NO_SPACE                = 12,
} anvil_result;

inline const char *anvil_result_string(const anvil_result res) {
    switch (res) {
    case ANVIL_OK: return "Ok";
    case ANVIL_TOO_SMALL: return "Insufficient space";
    case ANVIL_MALFORMED: return "Malformed";
    case ANVIL_ERROR_IO: return "IO Error";
    case ANVIL_INVALID_USAGE: return "Invalid usage";
    case ANVIL_ALLOC_FAILED: return "Alloc failed";
    case ANVIL_LOCKED: return "Locked";
    case ANVIL_NOT_EXIST: return "Does not exist";
    case ANVIL_DONE: return "Done";
    case ANVIL_UNSUPPORTED_COMPRESSION: return "Unsupported compression";
    case ANVIL_ERRNO: return strerror(errno);
    case ANVIL_NO_SPACE: return "No space";
    default: return "Invalid anvil_result";
    }
}

/**
 * Gets the error value.
 * This value is set by methods on error.
 * @return the current error value.
 */
anvil_result anvil_get_error();

/**
 * Sets the error value.
 * This is only useful for library makers who wish to preserve an original
 * error value for users while performing additional actions (i.e. closing anvil files)
 * that may set the error value to a new value.
 * @param result The new error value.
 */
void anvil_set_error(anvil_result result);

/**
 * Get a region file's name.
 * @param[out] name
 * @param[in] name_len maximum number of
 * @param[in] prefix Prefix to prepend to the filename. i.e. "r" in r.2.3.mca.
 * @param[in] x First coordinate in the filename.
 * @param[in] z Second coordinate in the filename.
 * @param[in] extension File extension to append to the filename. i.e. "mca" in r.2.3.mca.
 * @return Filename in region file format, or nullptr on memory allocation failure.
 *
 * @note The returned string remains owned by anvil_create_filename. Do not free it.
 *
 * @see @link anvil_parse_filename @endlink
 */
size_t anvil_create_filename(
    const char *name,
    size_t name_len,
    const char *prefix,
    int64_t x,
    int64_t z,
    const char *extension
);

/**
 * Parse a region file's name.
 * @param[in] name (nullable) filename to parse.
 * @param[out] prefix (nullable) Prefix to prepend to the filename. i.e. "r" in r.2.3.mca.
 * @param[out] x (nullable) First coordinate in the filename.
 * @param[out] z (nullable) Second coordinate in the filename.
 * @param[out] extension (nullable) File extension to append to the filename. i.e. "mca" in r.2.3.mca.
 * @return True if parsing was successful, false if unsuccessful.
 *
 * @see @link anvil_create_filename @endlink
 */
bool anvil_parse_filename(
    const char *name,
    const char **prefix,
    int64_t *x,
    int64_t *z,
    const char **extension
);

/**
 * parses position from a region file or chunk file name.
 * this method does not attempt to rigorously validate input names.
 *
 * @param[in] name region file or chunk file name or complete path.
 * @param[out] x_out (nullable) region x coordinate output.
 * @param[out] z_out (nullable) region z coordinate output.
 * @param[out] extension (nullable) region file extension.
 * @retval ANVIL_OK on success.
 * @retval ANVIL_INVALID_USAGE name could not be parsed.
 */
anvil_result anvil_parse_name(
    const char *name,
    int64_t *x_out,
    int64_t *z_out,
    char **extension
);

/**
 * anvil_world holds a handle to a world directory,
 * providing methods for interacting with worlds.
 *
 * @see anvil_world_open
 * @see anvil_world_close
 * @see anvil_world_region_dir_open
 */
typedef struct anvil_world anvil_world;

/**
 * region directory handle.
 * @see anvil_world_open_region_dir
 * @see anvil_region_dir_open
 * @see anvil_region_dir_close
 */
typedef struct anvil_dir anvil_dir;

/**
 * region file handle.
 * @see anvil_region_open_file
 * @see anvil_chunk_mtime
 * @see anvil_chunk_read
 * @see anvil_chunk_write
 * @see anvil_region_file_close
 */
typedef struct anvil_file anvil_file;

/**
 * Opens the anvil world using the given methods.
 *
 * @param[out] world_out Handle to the world.
 * @param[in] path Path to the world directory.
 *  It does not keep a reference to the string.
 *  e.g. /home/stewi/.minecraft/saves/New World
 * @param[in] alloc (nullable) Private memory allocation methods.
 *  If non-null all fields must be non-null and valid.
 * @retval ANVIL_OK On success.
 * @retval ANVIL_ALLOC_FAILED Memory allocation failed.
 * @retval ANVIL_LOCKED The world is already locked by another process.
 * @retval ANVIL_INVALID_USAGE Given arguments were invalid.
 * @retval ANVIL_NOT_EXIST The path does not exist.
 */
anvil_world *anvil_world_open(
    const char *path,
    anvil_allocator *alloc
);

/**
 * opens a directory containing region files in the world.
 *
 * it is equivalent to @link anvil_dir_open @endlink except
 * having an open world handle protects against concurrent access by multiple processes.
 *
 * @param[out] dir_out handle to the region directory. cannot be reused.
 * @param[in] world handle to the world.
 * @param[in] subdir (nullable) directory relative to the world directory to open.
 *  e.g. region, DIM1/entities
 * @param[in] region_extension (nullable) file extension for region files.
 *  it does not keep a reference to the string. defaults to "mca".
 * @param[in] chunk_extension (nullable) file extension for chunk files.
 *  it does not keep a reference to the string. defaults to "mcc".
 * @retval ANVIL_OK on success.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_INVALID_USAGE given arguments were invalid.
 * @retval ANVIL_NOT_EXIST the path does not exist.
 *
 * @see anvil_region_dir_open
 * @see anvil_region_dir_open_ex
 */
anvil_result anvil_world_open_dir(
    struct anvil_dir **dir_out,
    const struct anvil_world *world,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension
);

/**
 * anvil_close releases resources associated with the world.
 * @param[in] world handle to the world
 */
void anvil_world_close(struct anvil_world *world);

/**
 * opens a region directory using the given methods.
 *
 * it is recommended to use @link anvil_world_open_dir @endlink instead
 * as it provides safety from concurrent access by multiple processes.
 *
 * @param[out] dir_out handle to the region directory. cannot be reused.
 * @param[in] path path to the region directory.
 *  it does not keep a reference to the string.
 *  e.g. /home/stewi/.minecraft/saves/New World/region, /home/stewi/.minecraft/saves/New World/DIM1/entities.
 * @param[in] subdir (nullable) if non-null is appended to path to make the complete path to the region directory.
 * @param[in] region_extension (nullable) file extension for region files.
 *  it does not keep a reference to the string. defaults to "mca".
 * @param[in] chunk_extension (nullable) file extension for chunk files.
 *  it does not keep a reference to the string. defaults to "mcc".
 * @param[in] alloc (nullable) private memory allocation methods.
 *  if non-null all fields must be non-null and valid.
 * @retval ANVIL_OK on success.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_INVALID_USAGE given arguments were invalid.
 * @retval ANVIL_NOT_EXIST the path does not exist.
 *
 * @see anvil_world_region_dir_open
 */
anvil_result anvil_dir_open(
    struct anvil_dir **dir_out,
    const char *path,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,
    const anvil_allocator *alloc
);

/**
 * used for iterating over all region files in a directory.
 *
 * @see anvil_region_iter_open
 * @see anvil_region_iter_next
 * @see anvil_region_iter_close
 */
struct anvil_iter;
struct anvil_entry {
    const char *subdir;   /** path relative to save directory where the region file resides. i.e. "region", "DIM1/entities" */
    const char *filename; /** the region file's name. */
    int64_t region_x;     /** region x coordinate. */
    int64_t region_z;     /** region y coordinate. */
    time_t mtime;         /** the last modification time of the region. */
    off_t size;           /** size of the file in bytes. if the file is a symbolic link it is the length in bytes of the pathname contained in the symbolic link. */
};

/**
 * begins an iteration over region files in the directory.
 * @param[out] iter_out handle to the iteration
 * @param[in] dir region directory to iterate over
 * @retval ANVIL_OK on success.
 * @retval ANVIL_INVALID_USAGE given arguments were invalid.
 * @retval ANVIL_NOT_EXISTS the directory does not exist.
 *
 * @see anvil_region_iter_next
 * @see anvil_region_iter_close
 */
anvil_result anvil_iter_open(
    struct anvil_iter **iter_out,
    const struct anvil_dir *dir
);

/**
 * gets the next region file.
 * @param[out] entry handle to the entry to populate with details.
 * @param[in] iter handle to the iteration.
 * @retval ANVIL_NEXT successfully read the next region file.
 * @retval ANVIL_DONE the last region file was reached.
 * @retval ANVIL_INVALID_USAGE given arguments were invalid.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 *
 * @note This method opens and calls stat on the files internally.
 *  If the open or stat fails with
 *   @link EACCES @endlink,
 *   @link ELOOP @endlink,
 *   @link ENAMETOOLONG @endlink,
 *   @link ENOENT @endlink,
 *   @link EISDIR @endlink,
 *   @link EINVAL @endlink,
 *   @link EOVERFLOW @endlink or
 *   @link EFBIG @endlink
 *  the region file is skipped and no error is reported.
 *  Other errors are returned as normal.
 *
 * @see anvil_region_iter_open
 * @see anvil_region_iter_close
 */
anvil_result anvil_iter_next(
    struct anvil_entry *entry,
    struct anvil_iter *iter
);

/**
 * open the current region file in the iteration.
 * @param[out] file_out handle to the region file.
 * @param[int] iter handle to the iterator.
 * @retval ANVIL_OK on success.
 * @retval ANVIL_INVALID_USAGE given arguments were invalid.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 */
anvil_result anvil_iter_open_file(
    struct anvil_file **file_out,
    const struct anvil_iter *iter
);

/**
 * releases resources associated with the iteration.
 * @param[in] iter handle to the iteration.
 *
 * @see anvil_region_iter_open
 * @see anvil_region_iter_next
 */
void anvil_iter_close(
    struct anvil_iter *iter
);

/**
 * opens the region file for the given coordinates, creating it if it does not exist already.
 *
 * @param[out] file_out handle to the region file.
 * @param[in] dir directory to open the region file in.
 * @param[in] region_x region x coordinate
 * @param[in] region_z region z coordinate
 * @retval ANVIL_OK on success.
 * @retval ANVIL_INVALID_USAGE given arguments were invalid.
 * @retval ANVIL_NOT_EXIST a directory component in path does not exist or is not a directory.
 * @retval ANVIL_DISK_FULL region file does not exist and there is not enough space on the disk to create one.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 */
anvil_result anvil_open_file(
    struct anvil_file **file_out,
    const struct anvil_dir *dir,
    int64_t region_x,
    int64_t region_z
);

/**
 * permanently delete a region.
 * @param[in] dir handle to the region directory.
 * @param[in] region_x region x coordinate.
 * @param[in] region_z region z coordinate.
 * @retval ANVIL_OK on success.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 */
anvil_result anvil_remove(
    struct anvil_dir *dir,
    int64_t region_x,
    int64_t region_z
);

/**
 * releases resources associated with the region directory.
 * @param dir handle to the region directory.
 */
void anvil_dir_close(struct anvil_dir *dir);

/**
 * Get the last modification time of a chunk.
 * @param[in] file the region file. returns 0 if region_file is null.
 * @param[in] chunk_x chunk x coordinate. does not need to be relative to region coordinates.
 * @param[in] chunk_z chunk z coordinate. does not need to be relative to region coordinates.
 * @return last modification time of the chunk in epoch seconds.
 *  returns 0 if region_file is null, the region files has errored, or the chunk has never been written.
 */
uint32_t anvil_mtime(
    const struct anvil_file *file,
    int64_t chunk_x,
    int64_t chunk_z
);

/**
 * Read chunk data.
 *
 * Due to minecraft's unfortunate decision to incorrectly handle compression,
 * the decompressed size is *not* known ahead of time.
 * It also therefore follows that overflowing the decompressed buffer is *not* an error,
 * and instead needs to be explicitly handled under normal operation.
 *
 * Retrying this method in a loop when it returns ANVIL_TOO_SMALL and growing the
 * output buffer (to out_len is a good guess) is one of the few terrible approaches to resolving this shortcoming.
 *
 * Another terrible approach is to mmap the maximum theoretical size - an often truly huge region of virtual memory,
 * and let the operating system figure out what to do with your data.
 *
 * At the very least minecraft *does* use a checksum to validate the compressed data
 * as the ZLIB/GZIP format internally uses one, but I doubt that was a conscious decision.
 *
 * @param[out] out Output buffer where chunk data will be read into.
 * @param[in] out_cap Size of the output buffer.
 * @param[out] out_len (nullable) Size of data read into the output buffer.
 *  If ANVIL_INSUFFICIENT_SPACE is returned, out_len is set to the number of bytes that would have been
 *  read into out if it is known, or a reasonable guess of what the size might be if not.
 * @param[in] chunk_x Chunk x coordinate. does not need to be relative to region coordinates.
 * @param[in] chunk_z Chunk z coordinate. does not need to be relative to region coordinates.
 * @param[in] file File to read chunk data from.
 *
 * @retval ANVIL_OK On success.
 * @retval ANVIL_INVALID_USAGE If bad arguments are given or a previous error forced closing of the file to protect data integrity.
 * @retval ANVIL_INSUFFICIENT_SPACE If out is too small to hold the chunk data.
 * @retval ANVIL_MALFORMED The region file is corrupted.
 * @retval ANVIL_IO_ERROR An IO error occurred and errno is set.
 * @retval ANVIL_UNSUPPORTED_COMPRESSION The chunk was stored with an unsupported compression regime.
 *
 * @note If the chunk contains no data it returns ANVIL_OK and if out_len is not null sets it to 0.
 *
 * @note Use of the region file other than closing it after an error will return ANVIL_INVALID_USAGE in this and other methods.
 *
 * @note This method will cache buffers and/or files such that a failed read (i.e. due to ANVIL_INSUFFICIENT_SPACE)
 *  can be quickly continued.
 */
anvil_result anvil_read(
    void *restrict out,
    size_t out_cap,
    size_t *out_len,
    int64_t chunk_x,
    int64_t chunk_z,
    struct anvil_file *file
);

/**
 * Type of compression. ZLIB is the only compression algorithm commonly used in practice.
 *
 * The custom compression algorithm based on LZ4 that minecraft calls "LZ4" is not supported.
 */
typedef enum anvil_compression_e {
    ANVIL_COMPRESSION_NONE = 3,
    ANVIL_COMPRESSION_GZIP = 1,
    ANVIL_COMPRESSION_ZLIB = 2,
} anvil_compression;

/**
 * Write chunk data.
 * @param[in] in Input buffer containing data to be written.
 * @param[in] in_len Size of the data in the input buffer to be written.
 * @param[in] compression_level [0, 1] How much the data should be compressed at the expense of CPU time.
 * @param[in] compression Type of compression to use to compress the chunk.
 * @param[in] chunk_x Chunk x coordinate. does not need to be relative to region coordinates.
 * @param[in] chunk_z Chunk z coordinate. does not need to be relative to region coordinates.
 * @param[in] file File to write chunk data to.
 *
 * @retval ANVIL_OK on success.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 *
 * @note Use of the region file other than closing it after an error will return ANVIL_INVALID_USAGE in this and other methods.
 *
 * @note Changing the compression level between calls will cause the compression context to be recreated.
 */
anvil_result anvil_write(
    const void *restrict in,
    size_t in_len,
    double compression_level,
    anvil_compression compression,
    int64_t chunk_x,
    int64_t chunk_z,
    struct anvil_file *file
);

/**
 * Flushes changes made to the region file
 * and releases resources associated with the region file.
 * @param file handle to the region file.
 * @retval ANVIL_OK On success.
 * @retval ANVIL_IO_ERROR An IO error occurred and errno is set.
 *
 * @note This is the most likely place to receive file write errors from.
 *  Errors from this should be checked, if for no other reason than to log,
 *  instead of silently swallowing, write errors.
 *
 * @note This method will delete the file on disk if it contains no chunk data.
 */
anvil_result anvil_file_close(struct anvil_file *file);

/**
 * @}
 */
