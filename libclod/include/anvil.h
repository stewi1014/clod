#pragma once
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @defgroup anvil anvil.h
 *
 * @{
 * @file anvil.h
 * This header defines methods for read and writing to minecraft worlds.
 * @link anvil_world_open @endlink is the primary entrypoint,
 * with subsequent calls opening/manipulating particular attributes of the world.
 *
 * Methods avoid interdependency when possible, which means that methods for directly
 * creating a @link anvil_region_file @endlink handle or similar are exposed and their usage supported.
 * In some cases direct usage in this manner has some caveats which are documented.
 *
 */

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

/** the default allocator. if a null custom allocator is given, this is what is used. */
static const anvil_allocator default_anvil_allocator = {malloc, calloc, free, realloc};

#ifdef _WIN32
#define ANVIL_PATH_SEPERATOR "\\"
#else
#define ANVIL_PATH_SEPERATOR "/"
#endif

/**
 * place where messages are logged.
 *
 * messages are sparingly used, and only emitted with information that
 * the library has no other recourse for handling, and is relevant to an end user.
 *
 * can be set to null to disable messages.
 */
static FILE *anvil_messages = stderr;

/**
 * macro for creating file names for a given coordinate using snprintf.
 *
 * @param out buffer to write thr string into.
 * @param out_len maximum number of bytes to write into out.
 * @param path directory path to prepend.
 * @param prefix filename prefix. e.g. "r", "c".
 * @param x x coordinate.
 * @param z z coordinate.
 * @param extension file name extension.
 */
#define anvil_filepath(out, out_len, path, prefix, x, z, extension) \
    snprintf(out, out_len, "%s" ANVIL_PATH_SEPERATOR prefix ".%ld.%ld.%s", path, x, z, extension);

/**
 * used to describe the result of operations
 */
typedef enum anvil_result_e {
    ANVIL_OK                      = 1,  /** operation was successful. */
    ANVIL_INSUFFICIENT_SPACE      = 2,  /** the given buffer is not large enough to hold the requested data. */
    ANVIL_MALFORMED               = 3,  /** input data is malformed. */
    ANVIL_IO_ERROR                = 4,  /** an IO error occurred, and errno should be set. */
    ANVIL_INVALID_ARGUMENT        = 5,  /** argument to method was invalid. */
    ANVIL_ALLOC_FAILED            = 6,  /** memory allocation failed. */
    ANVIL_LOCKED                  = 7,  /** the resource is locked. */
    ANVIL_NOT_EXIST               = 8,  /** the resource does not exist. */
    ANVIL_NEXT                    = 9,  /** this part of a multipart operation was successful. more calls can be made. */
    ANVIL_DONE                    = 10, /** the multipart operation is finished. no more calls should be made. */
    ANVIL_DISK_FULL               = 11, /** storage device is full. */
    ANVIL_INVALID_NAME            = 12, /** name is not valid. */
    ANVIL_UNSUPPORTED_COMPRESSION = 13, /** compression regime is not supported. */
} anvil_result;

static const char* anvil_result_string(const anvil_result res) {
    switch (res) {
    case 0: return "Uninitialised Result";
    case ANVIL_OK: return "Ok";
    case ANVIL_INSUFFICIENT_SPACE: return "Insufficient space";
    case ANVIL_MALFORMED: return "Malformed";
    case ANVIL_IO_ERROR: return strerror(errno);
    case ANVIL_INVALID_ARGUMENT: return "Invalid argument";
    case ANVIL_ALLOC_FAILED: return "Alloc failed";
    case ANVIL_LOCKED: return "Locked";
    case ANVIL_NOT_EXIST: return "Does not exist";
    case ANVIL_NEXT: return "Next";
    case ANVIL_DONE: return "Done";
    case ANVIL_DISK_FULL: return "Disk full";
    case ANVIL_INVALID_NAME: return "Invalid name";
    case ANVIL_UNSUPPORTED_COMPRESSION: return "Unsupported compression";
    default: return "Invalid result";
    }
}

//=============//
// Anvil World //
//=============//

/**
 * anvil_world holds a handle to a world directory,
 * providing methods for interacting with worlds.
 *
 * @see anvil_world_open
 * @see anvil_world_open_ex
 * @see anvil_world_close
 * @see anvil_world_region_dir_open
 */
struct anvil_world;

/**
 * opens the anvil world using the given methods.
 *
 * @param[out] world_out handle to the world. cannot be reused.
 * @param[in] path path to the world directory.
 *  it does not keep a reference to the string.
 *  e.g. /home/stewi/.minecraft/saves/New World
 * @param[in] alloc (nullable) private memory allocation methods.
 *  if non-null all fields must be non-null and valid.
 * @retval ANVIL_OK on success.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_LOCKED the world is already locked by another process.
 * @retval ANVIL_INVALID_ARGUMENT given arguments were invalid.
 * @retval ANVIL_NOT_EXIST the path does not exist.
 */
anvil_result anvil_world_open(
    struct anvil_world **world_out,
    const char *path,
    anvil_allocator *alloc
);

/**
 * opens a directory containing region files in the world.
 *
 * it is equivalent to @link anvil_region_dir_open @endlink except
 * having an open world handle protects against concurrent access by multiple processes.
 *
 * @param[out] region_dir_out handle to the region directory. cannot be reused.
 * @param[in] world handle to the world.
 * @param[in] subdir (nullable) directory relative to the world directory to open.
 *  e.g. region, DIM1/entities
 * @param[in] region_extension (nullable) file extension for region files.
 *  it does not keep a reference to the string. defaults to "mca".
 * @param[in] chunk_extension (nullable) file extension for chunk files.
 *  it does not keep a reference to the string. defaults to "mcc".
 * @retval ANVIL_OK on success.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_INVALID_ARGUMENT given arguments were invalid.
 * @retval ANVIL_NOT_EXIST the path does not exist.
 *
 * @see anvil_region_dir_open
 * @see anvil_region_dir_open_ex
 */
anvil_result anvil_world_open_region_dir(
    struct anvil_region_dir **region_dir_out,
    struct anvil_world *world,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension
);

/**
 * anvil_close releases resources associated with the world.
 * @param[in] world handle to the world
 */
void anvil_world_close(struct anvil_world *world);

//==============//
// Anvil Region //
//==============//

/**
 * region directory handle.
 * @see anvil_world_open_region_dir
 * @see anvil_region_dir_open
 * @see anvil_region_dir_close
 */
struct anvil_region_dir;

/**
 * opens a region directory using the given methods.
 *
 * it is recommended to use @link anvil_world_open_region_dir @endlink instead
 * as it provides safety from concurrent access by multiple processes.
 *
 * @param[out] region_dir_out handle to the region directory. cannot be reused.
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
 * @retval ANVIL_INVALID_ARGUMENT given arguments were invalid.
 * @retval ANVIL_NOT_EXIST the path does not exist.
 *
 * @see anvil_world_region_dir_open
 */
anvil_result anvil_region_dir_open(
    struct anvil_region_dir **region_dir_out,
    const char *path,
    const char *subdir,
    const char *region_extension,
    const char *chunk_extension,
    anvil_allocator *alloc
);

/**
 * used for iterating over all region files in a directory.
 *
 * @see anvil_region_iter_open
 * @see anvil_region_iter_next
 * @see anvil_region_iter_close
 */
struct anvil_region_iter;
struct anvil_region_entry {
    char *path;        /** path to the region file. */
    char *subdir;      /** path relative to save directory where the region file resides. */
    char *filename;    /** the region file's name. */
    int64_t region_x;  /** region x coordinate. */
    int64_t region_z;  /** region y coordinate. */
    time_t mtime;      /** the last modification time of the region. */
    off_t size;        /** size of the file in bytes. if the file is a symbolic link it is the length in bytes of the pathname contained in the symbolic link. */
    blksize_t blksize; /** preferred IO block size for the file. */
    blkcnt_t blkcnt;   /** number of blocks allocated for the file. */
};

/**
 * begins an iteration over region files in the directory.
 * @param[out] region_iter_out handle to the iteration
 * @param[in] region_dir region directory to iterate over
 * @retval ANVIL_OK on success.
 * @retval ANVIL_INVALID_ARGUMENT given arguments were invalid.
 * @retval ANVIL_NOT_EXISTS the directory does not exist.
 *
 * @see anvil_region_iter_next
 * @see anvil_region_iter_close
 */
anvil_result anvil_region_iter_open(
    struct anvil_region_iter **region_iter_out,
    const struct anvil_region_dir *region_dir
);

/**
 * gets the next region file.
 * @param[out] entry handle to the entry to populate with details.
 * @param[in] region_iter handle to the iteration.
 * @retval ANVIL_NEXT successfully read the next region file.
 * @retval ANVIL_DONE the last region file was reached.
 * @retval ANVIL_INVALID_ARGUMENT given arguments were invalid.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 *
 * @note this method calls stat internally on files found in the directory.
 *  if the stat fails with
 *   @link EACCES @endlink,
 *   @link ELOOP @endlink,
 *   @link ENAMETOOLONG @endlink,
 *   @link ENOENT @endlink or
 *   @link EOVERFLOW @endlink
 *  the region file is skipped,
 *  and the method returns ANVIL_OK.
 *  other errors are returned as normal.
 *
 * @see anvil_region_iter_open
 * @see anvil_region_iter_close
 */
anvil_result anvil_region_iter_next(
    struct anvil_region_entry *entry,
    struct anvil_region_iter *region_iter
);

/**
 * releases resources associated with the iteration.
 * @param[in] region_iter handle to the iteration.
 *
 * @see anvil_region_iter_open
 * @see anvil_region_iter_next
 */
void anvil_region_iter_close(
    struct anvil_region_iter *region_iter
);

/**
 * parses position from a region file or chunk file name.
 * this method does not attempt to rigorously validate input names.
 *
 * @param[in] name region file or chunk file name or complete path.
 * @param[out] region_x_out region x coordinate output.
 * @param[out] region_z_out region z coordinate output.
 * @retval ANVIL_OK on success.
 * @retval ANVIL_INVALID_NAME name could not be parsed.
 */
anvil_result anvil_region_parse_name(
    const char *name,
    int64_t *region_x_out,
    int64_t *region_z_out
);

/**
 * opens the region file for the given coordinates, creating it if it does not exist already.
 *
 * @param[out] region_file_out handle to the region file.
 * @param[in] region_dir directory to open the region file in.
 * @param[in] region_x region x coordinate
 * @param[in] region_z region z coordinate
 * @retval ANVIL_OK on success.
 * @retval ANVIL_INVALID_ARGUMENT given arguments were invalid.
 * @retval ANVIL_NOT_EXIST a directory component in path does not exist or is not a directory.
 * @retval ANVIL_DISK_FULL region file does not exist and there is not enough space on the disk to create one.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 */
anvil_result anvil_region_open_file(
    struct anvil_region_file **region_file_out,
    struct anvil_region_dir *region_dir,
    int64_t region_x,
    int64_t region_z
);

/**
 * permanently delete a region.
 * @param[in] region_dir handle to the region directory.
 * @param[in] region_x region x coordinate.
 * @param[in] region_z region z coordinate.
 * @retval ANVIL_OK on success.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 */
anvil_result anvil_region_remove(
    struct anvil_region_dir *region_dir,
    int64_t region_x,
    int64_t region_z
);

/**
 * releases resources associated with the region directory.
 * @param region_dir handle to the region directory.
 */
void anvil_region_dir_close(struct anvil_region_dir *region_dir);

/**
 * region file handle
 * @see anvil_region_open_file
 * @see anvil_region_file_open
 * @see anvil_chunk_mtime
 * @see anvil_chunk_read
 * @see anvil_chunk_write
 * @see anvil_region_file_close
 */
struct anvil_region_file;

/**
 * opens a file in the region directory.
 * if the region file does not exist it is created.
 *
 * it is recommended to use @link anvil_region_open_file @endlink instead,
 * as it provides a more helpful interface,
 * and can offer safety from concurrent access by multiple processes.
 *
 * @param[out] region_file_out handle to the region file.
 * @param[in] path path to the region directory, *not* the file. it does not keep a reference to the string.
 * @param[in] chunk_extension (nullable) file extension for chunk files.
 *  it does not keep a reference to the string. defaults to "mcc".
 * @param[in] alloc (nullable) private memory allocation methods.
 *  if non-null all fields must be non-null and valid.
 * @retval ANVIL_OK on success.
 * @retval ANVIL_INVALID_ARGUMENT given arguments were invalid.
 * @retval ANVIL_NOT_EXIST a directory component in path does not exist or is not a directory.
 * @retval ANVIL_DISK_FULL region file does not exist and there is not enough space on the disk to create one.
 * @retval ANVIL_ALLOC_FAILED memory allocation failed.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 * @retval AnVIL_INVALID_NAME the region file coordinates could not be parsed from the file name.
 */
anvil_result anvil_region_file_open(
    struct anvil_region_file **region_file_out,
    const char *path,
    const char *chunk_extension,
    const anvil_allocator *alloc
);

/**
 * Get the last modification time of a chunk.
 * @param[in] region_file the region file. returns 0 if region_file is null.
 * @param[in] chunk_x chunk x coordinate. does not need to be relative to region coordinates.
 * @param[in] chunk_z chunk z coordinate. does not need to be relative to region coordinates.
 * @return last modification time of the chunk in epoch seconds.
 *  returns 0 if region_file is null, the region files has errored, or the chunk has never been written.
 */
uint32_t anvil_chunk_mtime(
    const struct anvil_region_file *region_file,
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
 * Retrying this method in a loop when it returns ANVIL_INSUFFICIENT_SPACE and growing the
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
 * @param[in] region_file File to read chunk data from.
 *
 * @retval ANVIL_OK On success.
 * @retval ANVIL_INSUFFICIENT_SPACE If out is too small to hold the chunk data.
 * @retval ANVIL_MALFORMED The region file is corrupted.
 * @retval ANVIL_IO_ERROR An IO error occurred and errno is set.
 * @retval ANVIL_UNSUPPORTED_COMPRESSION The chunk was stored with an unsupported compression regime.
 *
 * @note If the chunk contains no data it returns ANVIL_OK and if out_len is not null sets it to 0.
 *
 * @note Use of the region file other than closing it after an error will return ANVIL_INVALID_ARGUMENT in this and other methods.
 *
 * @note This method will cache buffers and/or files such that a failed read (i.e. due to ANVIL_INSUFFICIENT_SPACE)
 *  can be quickly continued.
 */
anvil_result anvil_chunk_read(
    void *restrict out,
    size_t out_cap,
    size_t *out_len,
    int64_t chunk_x,
    int64_t chunk_z,
    struct anvil_region_file *region_file
);

/**
 * Type of compression
 *
 * The unstandardised compression algorithm based on LZ4 that minecraft calls "LZ4" is not supported.
 */
typedef enum anvil_compression_e {
    ANVIL_COMPRESSION_NONE = 3,
    ANVIL_COMPRESSION_GZIP = 1,
    ANVIL_COMPRESSION_ZLIB = 2,
} anvil_compression;

/**
 * write chunk data.
 * @param[in] in input buffer containing data to be written.
 * @param[in] in_len size of the data in the input buffer to be written.
 * @param[in] compression type of compression to use to compress the chunk.
 * @param[in] chunk_x chunk x coordinate. does not need to be relative to region coordinates.
 * @param[in] chunk_z chunk z coordinate. does not need to be relative to region coordinates.
 * @param[in] region_file file to write chunk data to.
 *
 * @retval ANVIL_OK on success.
 * @retval ANVIL_IO_ERROR an IO error occurred and errno is set.
 *
 * @note use of the region file other than closing it after an error will return ANVIL_INVALID_ARGUMENT in this and other methods.
 */
anvil_result anvil_chunk_write(
    const void *restrict in,
    size_t in_len,
    enum anvil_compression compression,
    int64_t chunk_x,
    int64_t chunk_z,
    struct anvil_region_file *region_file
);

/**
 * releases resources associated with the region file.
 * @param region_file handle to the region file.
 */
void anvil_region_file_close(struct anvil_region_file *region_file);

/**
 * @}
 */

/*

struct anvil_world;

//===============//
// World Reading //
//===============//

/**
 * same as anvil_open_ex, but reference function implementations are used.
 *
struct anvil_world *anvil_open(const char *path);

/**
 * opens an anvil-format minecraft world.
 * 
 * path is the path to the world folder, or level.dat file.
 * 
 * when opening a world, a copy of the path is made,
 * and if the path ends in "level.dat" it is trimmed.
 * 
 * any of the following function arguments may be null,
 * and their default implementation will be used instead.
 * 
 * read_dir must return a null-terminated array of null-terminated
 * filenames for files in the given directory, or NULL on error.
 * the filenames and array are passed to the free function when no longer needed.
 * 
 * open_file must return a buffer containing the file's contents,
 * and set size to the size of the buffered data, or NULL on error.
 * the idea behind char** over char*, is that an intermediate structure
 * with metadata can be returned by open_file which is then available in the close_file method.
 * 
 * close_file is called when the file is no longer needed.
 * it is always given the nonnull return value of read_file.
 * 
 * realloc_f must behave like the realloc function.
 *
struct anvil_world *anvil_open_ex(
    const char *path,
    int scandir(
        const char *restrict dirp,
        struct dirent ***restrict namelist,
        typeof(int (const struct dirent *)) *filter,
        typeof(int (const struct dirent **, const struct dirent **)) *comparator
    ),
    char **(*open_file_f)(const char *path, size_t *size),
    void (*close_file_f)(char **file),
    void *(*realloc_f)(void*, size_t)
);

/**
 * releases resources associated with the anvil world.
 *
void anvil_close(struct anvil_world *);



//================//
// Region Reading //
//================//

/**
 * The idea behind region reading is to give the user guidance on what region files to read.
 * Implementing a getter API here doesn't work as the user doesn't know what regions actually exist in the world.
 * You can't much iterate over every possible region file that might exist.
 * 
 * So an iterator that allows the user to visit every region file is made available.
 * If a particular order of visitation is required then some new iter method/s can be added
 * that facilitate this custom iteration order - but perhaps it's better to reconsider
 * whatever use case it is that depends on iterating over files in a particular order...
 *

struct anvil_region {
    char *data;
    size_t data_size;
    int64_t region_x;
    int64_t region_z;
};

struct anvil_region anvil_region_new(
    char *data,
    size_t data_size,
    int64_t region_x,
    int64_t region_z
);

struct anvil_region_iter;
struct anvil_region_iter *anvil_region_iter_new(const char *subdir, const struct anvil_world *world);
void anvil_region_iter_free(struct anvil_region_iter *);
int anvil_region_iter_next(
    struct anvil_region *,
    struct anvil_region_iter *
);



//===============//
// Chunk Reading //
//===============//

/**
 * The general idea behind chunk reading is that anvil_chunk_decompress uses the persisting
 * resources (large allocated buffers & decompressors) in anvil_chunk_ctx to provide
 * a transient view into chunk data that is only valid until the next call with the same context (resources are then reused).
 * 
 * This means that only one chunk can be loaded at a time, except it doesn't.
 * If you need to load 4 chunks at once you can create 4 anvil_chunk_ctx's and use them without any special considerations.
 *
struct anvil_chunk_ctx;
struct anvil_chunk_ctx *anvil_chunk_ctx_alloc(const struct anvil_world *);
void anvil_chunk_ctx_free(struct anvil_chunk_ctx *);

struct anvil_chunk {
    char *data;
    size_t data_size;
    int64_t chunk_x;
    int64_t chunk_z;
};

struct anvil_chunk anvil_chunk_decompress(
    struct anvil_chunk_ctx *,
    const struct anvil_region *,
    int64_t chunk_x,
    int64_t chunk_z
);



//=================//
// Section Reading //
//=================//

struct anvil_section {                  // contains pointers to NBT data for a specific section.
    char *end;                          // the end of the section's NBT data, and start of next section.
    char *block_state_palette;          // list NBT payload. list of blocks (compound).
    uint16_t *block_state_indices;     // 16x16x16 array of block state indices. inaccurate if block state palette size is <= 1
    char *biome_palette;                // list NBT payload. list of biomes (string).
    uint16_t *biome_indices;           // 4x4x4 array of biome indices. inaccurate if biome palette size is <= 1
    char *block_light;                  // 2048 byte array
    char *sky_light;                    // 2048 byte array
};

struct anvil_sections {
    struct anvil_section *section;      // array of sections.
    int64_t len;                        // the number of sections in the array.
    int64_t cap;                        // the size (in sections) of the allocated array.

    int64_t x;
    int64_t z;
    int64_t min_y;                      // the lowest section y value. can be added to index to get section Y.

    char *status;                       // Chunk Status NBT tag.
    char *start;                        // the start of the first section's data.
    void *(*realloc)(void*, size_t);    // the method used to allocate section data.
};

#define ANVIL_SECTIONS_CLEAR (struct anvil_sections){NULL, 0, 0}

/**
 * parses the sections in the chunk into an array.
 * 
 * the array is sorted in descending y order.
 * 
 * sections must be ANVIL_SECTIONS_CLEAR, or the result of a previous call to anvil_arse_sections.
 * the caller is responsible for freeing the array when done.
 * 
 * it returns nonzero on failure.
 *
int anvil_parse_sections_ex(
    struct anvil_sections *sections,
    struct anvil_chunk,
    void *(*realloc_f)(void*, size_t)
);

/**
 * parses the sections in the chunk into an array.
 * 
 * the array is sorted in descending y order.
 * 
 * sections must be ANVIL_SECTIONS_CLEAR, or the result of a previous call to anvil_arse_sections.
 * the caller is responsible for freeing the array when done.
 * 
 * it returns nonzero on failure.
 *
static inline
int anvil_parse_sections(
    struct anvil_sections *sections,
    const struct anvil_chunk chunk
) {
    return anvil_parse_sections_ex(sections, chunk, NULL);
}

/** releases resources associated with the sections.
void anvil_sections_free(
    struct anvil_sections *sections
);
