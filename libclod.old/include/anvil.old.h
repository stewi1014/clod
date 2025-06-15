/**
 * @defgroup anvil anvil.h
 *
 * @example anvil_iter.c

 * @file anvil.h
 * This header defines methods for read and writing to minecraft worlds.
 * @link anvil_open @endlink is the primary entrypoint.
 */

#pragma once
#include <stdint.h>
#include <time.h>

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
 */
typedef enum anvil_result {
    /** Operation was successful. */
    ANVIL_OK                      = 1,
    /** The object is too small. */
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
    /** Other error that does not have dedicated value occurred. See errno and strerror. */
    ANVIL_OTHER                   = 11,
    /** The disk no space remaining. */
    ANVIL_NO_SPACE                = 12,
} anvil_result;

/**
 * Get a string value for the result.
 * @param[in] res the result to get the string value for.
 * @return String value.
 */
const char *anvil_strerror(anvil_result res);

/**
 * Not really private - library users *are* supposed to use this, just not directly.
 * And they're certainly not supposed to keep the address -
 * the macro is far more intuitive for C users, and bindings should only touch the getter.
 * @private
 */
anvil_result *__anvil_errno_location();

/**
 * The error value used in this library.
 * It is thread local.
 */
#define anvil_errno (*__anvil_errno_location())

/**
 * Gets the error value.
 * This value is set by methods on error.
 * @return the current error value.
 *
 * @note C code would probably prefer to use the @link anvil_errno @endlink macro.
 */
anvil_result anvil_get_error();

/**
 * Parse a region filename into components.
 * @param[in] name Filename to parse.
 * @param[in] name_size Size of the name.
 * @param[out] prefix_size_ptr Size of the unparsed prefix. i.e. 4 for "abcd" if given "abcd.-2.3.mca".
 * @param[out] extension_ptr Name suffix not including first '.'. i.e. "mca.gz" for "r.-2.3.mca.gz".
 * @param[in] num_coords Number of coordinates passed to the method. For minecraft region files this is always 2.
 *  If the number of coordinates in the filename exceeds num_coords the remaining coords are included in the prefix.
 * @param[out] ... Pointers to int64_t coordinates.
 * @return Number of parsable coordinates in the name. For minecraft region files this is always 2.
 */
int64_t anvil_parse_filename(
    const char *name,
    size_t name_size,
    size_t *prefix_size_ptr,
    const char **extension_ptr,
    int64_t num_coords,
    ... /** int64_t *coord... */
);

/**
 * Create a region filename from components.
 * @param[out] name Buffer to write the filename into.
 * @param[in] name_size Maximum number of bytes to write into name, including terminating '\0'.
 * @param[in] prefix Name prefix not including trailing '.'. i.e. "abcd" would produce "abcd.-2.3.mca".
 * @param[in] extension Name suffix not including first '.'. i.e. "mca.gz" for "r.-2.3.mca.gz".
 * @param[in] num_coords Number of coordinates passed to the method. For minecraft region files this is always 2.
 * @param[in] ... int64_t coordinates.
 * @return Number of bytes written into name, or 0 if extension or prefix would cause misinterpretation of coordinates.
 */
size_t anvil_create_filename(
    char *name,
    size_t name_size,
    const char *prefix,
    const char *extension,
    int64_t num_coords,
    ... /** int64_t coord... */
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
 * Opens the anvil world at the given path,
 * optionally using a private memory allocator.
 *
 * @param[in] path Path to the world directory.
 *  It does not keep a reference to the string.
 *  e.g. /home/stewi/.minecraft/saves/New World
 * @param[in] alloc (nullable) Private memory allocation methods.
 *  If non-null all fields must be non-null and valid.
 * @throws ANVIL_OK On success.
 * @throws ANVIL_ALLOC_FAILED Memory allocation failed.
 * @throws ANVIL_LOCKED The world is already locked by another process.
 * @throws ANVIL_INVALID_USAGE Given arguments were invalid.
 * @throws ANVIL_NOT_EXIST The path does not exist.
 */
anvil_world *anvil_open(
    const char *path,
    const anvil_allocator *alloc
);

struct anvil_dir_opts {
    char *region_prefix;
    char *region_extension;
    size_t region_coords_count;
    int64_t *region_coords_;

    char *chunk_prefix;
    char *chunk_extension;
};

/**
 * opens a directory containing region files in the world.
 *
 * it is equivalent to @link anvil_open_dir_direct @endlink except
 * having an open world handle protects against concurrent access by multiple processes.
 *
 * @param[in] world Handle to the world.
 * @param[in] subdir (nullable) Directory relative to the world directory to open.
 *  e.g. region, DIM1/entities
 * @param[in] region_coordinates Number of coordinates in region filenames.
 *  This is always 2 for minecraft region files.
 * @param[in] region_extension (nullable) File extension for region files.
 *  It does not keep a reference to the string. defaults to "mca".
 * @param[in] chunk_coordinates Number of chunk coordinates.
 *  This is always 2 for minecraft region files.
 * @param[in] chunk_extension (nullable) File extension for chunk files.
 *  it does not keep a reference to the string. defaults to "mcc".
 * @throws ANVIL_OK On success.
 * @throws ANVIL_ALLOC_FAILED Memory allocation failed.
 * @throws ANVIL_INVALID_USAGE Given arguments were invalid.
 * @throws ANVIL_NOT_EXIST The path does not exist.
 *
 * @see anvil_open_dir_direct
 */
anvil_dir *anvil_open_dir(
    const anvil_world *world,
    const char *subdir,
    size_t region_coordinates,
    const char *region_extension,
    size_t chunk_coordinates,
    const char *chunk_extension
);

/**
 * anvil_close releases resources associated with the world.
 * @param[in] world handle to the world
 */
void anvil_close(anvil_world *world);

/**
 * opens a region directory using the given methods.
 *
 * it is recommended to use @link anvil_open_dir @endlink instead
 * as it provides safety from concurrent access by multiple processes.
 *
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
 * @throws ANVIL_OK on success.
 * @throws ANVIL_ALLOC_FAILED memory allocation failed.
 * @throws ANVIL_INVALID_USAGE given arguments were invalid.
 * @throws ANVIL_NOT_EXIST the path does not exist.
 *
 * @see anvil_open_dir
 */
anvil_dir *anvil_open_dir_direct(
    const char *path,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,
    const anvil_allocator *alloc
);

/**
 * used for iterating over all region files in a directory.
 *
 * @see anvil_iter_open
 * @see anvil_iter_next
 * @see anvil_iter_close
 */
typedef struct anvil_iter anvil_iter;
struct anvil_entry {
    const char *subdir;   /** Path relative to save directory where the region file resides. i.e. "region", "DIM1/entities" */
    const char *filename; /** Region file's name. */
    time_t mtime;         /** Last modification time of the region. */
    size_t size;          /** Size of the file in bytes. if the file is a symbolic link it is the length in bytes of the pathname contained in the symbolic link. */
    int64_t *coords;      /** Coordinates of the region file. For minecraft region files it always has a length of 2. */
};

/**
 * begins an iteration over region files in the directory.
 * @param[in] dir region directory to iterate over
 * @throws ANVIL_OK on success.
 * @throws ANVIL_INVALID_USAGE given arguments were invalid.
 * @throws ANVIL_NOT_EXIST the directory does not exist.
 *
 * @see anvil_region_iter_next
 * @see anvil_region_iter_close
 */
anvil_iter *anvil_open_iter(
    const anvil_dir *dir
);

/**
 * gets the next region file.
 * @param[out] entry handle to the entry to populate with details.
 * @param[in] iter handle to the iteration.
 * @throws ANVIL_OK successfully read the next region file.
 * @throws ANVIL_DONE the last region file was reached.
 * @throws ANVIL_INVALID_USAGE given arguments were invalid.
 * @throws ANVIL_ALLOC_FAILED memory allocation failed.
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
    anvil_iter *iter,
    struct anvil_entry *entry
);

/**
 * open the current region file in the iteration.
 * @param[int] iter handle to the iterator.
 * @throws ANVIL_OK on success.
 * @throws ANVIL_INVALID_USAGE given arguments were invalid.
 * @throws ANVIL_ALLOC_FAILED memory allocation failed.
 */
anvil_file *anvil_iter_open_file(
    const anvil_iter *iter
);

/**
 * releases resources associated with the iteration.
 * @param[in] iter handle to the iteration.
 *
 * @see anvil_region_iter_open
 * @see anvil_region_iter_next
 */
void anvil_close_iter(
    anvil_iter *iter
);

/**
 * opens the region file for the given coordinates, creating it if it does not exist already.
 *
 * @param[in] dir directory to open the region file in.
 * @param[in] region_x region x coordinate
 * @param[in] region_z region z coordinate
 * @throws ANVIL_OK on success.
 * @throws ANVIL_INVALID_USAGE given arguments were invalid.
 * @throws ANVIL_NOT_EXIST a directory component in path does not exist or is not a directory.
 * @throws ANVIL_NO_SPACE region file does not exist and there is not enough space on the disk to create one.
 * @throws ANVIL_ALLOC_FAILED memory allocation failed.
 */
anvil_file *anvil_open_file(
    const anvil_dir *dir,
    int64_t region_x,
    int64_t region_z
);

/**
 * permanently delete a region.
 * @param[in] dir handle to the region directory.
 * @param[in] region_x region x coordinate.
 * @param[in] region_z region z coordinate.
 * @throws ANVIL_OK on success.
 * @throws ANVIL_ALLOC_FAILED memory allocation failed.
 */
anvil_result anvil_remove(
    anvil_dir *dir,
    int64_t region_x,
    int64_t region_z
);

/**
 * releases resources associated with the region directory.
 * @param dir handle to the region directory.
 */
void anvil_close_dir(anvil_dir *dir);

/**
 * Get the last modification time of a chunk.
 * @param[in] file the region file. returns 0 if region_file is null.
 * @param[in] chunk_x chunk x coordinate. does not need to be relative to region coordinates.
 * @param[in] chunk_z chunk z coordinate. does not need to be relative to region coordinates.
 * @return last modification time of the chunk in epoch seconds.
 *  returns 0 if region_file is null, the region files has errored, or the chunk has never been written.
 */
uint32_t anvil_mtime(
    const anvil_file *file,
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
 *  If ANVIL_TOO_SMALL is returned, out_len is set to the number of bytes that would have been
 *  read into out if it is known, or a reasonable guess of what the size might be if not.
 * @param[in] chunk_x Chunk x coordinate. does not need to be relative to region coordinates.
 * @param[in] chunk_z Chunk z coordinate. does not need to be relative to region coordinates.
 * @param[in] file File to read chunk data from.
 *
 * @throws ANVIL_OK On success.
 * @throws ANVIL_INVALID_USAGE If bad arguments are given or a previous error forced closing of the file to protect data integrity.
 * @throws ANVIL_TOO_SMALL If out is too small to hold the chunk data.
 * @throws ANVIL_MALFORMED The region file is corrupted.
 * @throws ANVIL_UNSUPPORTED_COMPRESSION The chunk was stored with an unsupported compression regime.
 *
 * @note If the chunk contains no data it returns ANVIL_OK and if out_len is not null sets it to 0.
 *
 * @note Use of the region file other than closing it after an error will return ANVIL_INVALID_USAGE in this and other methods.
 *
 * @note This method will cache buffers and/or files such that a failed read (i.e. due to ANVIL_TOO_SMALL)
 *  can be quickly continued.
 */
anvil_result anvil_read(
    void *restrict out,
    size_t out_cap,
    size_t *out_len,
    int64_t chunk_x,
    int64_t chunk_z,
    anvil_file *file
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
 * @throws ANVIL_OK on success.
 * @throws ANVIL_INVALID_USAGE If bad arguments are given or a previous error forced closing of the file to protect data integrity.
 * @throws ANVIL_NO_SPACE The disk is read only or has run out of space.
 * @throws ANVIL_MALFORMED The region file is corrupted.
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
    anvil_file *file
);

/**
 * Flushes changes made to the region file
 * and releases resources associated with the region file.
 * @param file handle to the region file.
 * @throws ANVIL_OK On success.
 *
 * @note This is the most likely place to receive file write errors from.
 *  Errors from this should be checked, if for no other reason than to log,
 *  instead of silently swallowing, write errors.
 *
 * @note This method will delete the file on disk if it contains no chunk data.
 */
anvil_result anvil_close_file(anvil_file *file);
