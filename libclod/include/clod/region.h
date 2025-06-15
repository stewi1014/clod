/**
 * @file clod/region.h
 * @defgroup region Region storage
 * @{
 *
 * region.h defines the public interface libclod exposes for interacting with a
 * gentrified and extended version of the minecraft region file format that retains compatibility with minecraft.
 *
 * https://minecraft.wiki/w/Region_file_format
 *
 * libclod extends the region file format with a header supporting dynamic features and uses
 * that support to add checksums and uncompressed chunk size to
 *
 * Thanks to [ishland](https://github.com/ishland) for his insights while brainstorming approaches to this.
 */

#ifndef CLOD_REGION_H
#define CLOD_REGION_H

#include <clod/lib.h>
#include <stdint.h>
#include <time.h>

/** Library ABI version. */
#define CLOD_REGION_VERSION 1

/** A region storage instance, enabling interaction with region files. */
struct clod_region;
/** Configuration parameters for the region storage.
 * Field zero values imply defaults. */
struct clod_region_opts;
/** Used to iterate over existing chunks in a region storage. */
struct clod_region_iter;

/**
 * Result of a call to a libregion library method.
 * It's designed to allow programs to respond to error states and does not intend to represent debug information.
 * The library currently uses stderr for debug information and user-relevant messages (i.e. permission denied).
 * I do realise that might not be ideal for some cases,
 * so I've tried to leave space to add support for a custom logger if such a thing becomes wanted.
 */
enum clod_region_result {
	/** No worries mate. */
	CLOD_REGION_OK = 0,
	/** Library used incorrectly - either directly or indirectly.
	 * File permissions errors, IO errors, system clock errors, virtual memory errors,
	 * memory allocation failures, invalid options, and many others all fall under this value. */
	CLOD_REGION_INVALID_USAGE = 1,
	/** Data is corrupted. Manual intervention is required.
	 * The program can delete the chunk to continue operation.
	 * @note Deleting a corrupted chunk can cause the deletion of other corrupted chunks. */
	CLOD_REGION_MALFORMED = 2,
	/** The chunk does not exist.
	 * The program can write to the chunk to make it exist. */
	CLOD_REGION_NOT_FOUND = 3
};

/**
 * Open a directory for region storage.
 * @param path Path to the directory.
 * @param opts Configuration options.
 * @return Handle to the directory and configuration, or nullptr on error.
 */
CLOD_API CLOD_USE_RETURN
struct clod_region *
clod_region_open(const char *path, const struct clod_region_opts *opts);

/**
 * Read chunk data.
 * @param[in] region Region handle.
 * @param[in] pos Chunk position.
 * @param[in] buff The buffer where data is written to.
 * @param[in] buff_size Size of dst.
 * @param[out] size Actual size of the chunk data.
 * @throws CLOD_REGION_OK On success.
 * @throws CLOD_REGION_INVALID_USAGE On invalid usage.
 * @throws CLOD_REGION_MALFORMED Chunk data is corrupted.
 * @throws CLOD_REGION_NOT_FOUND Chunk does not exist.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2, 3)
enum clod_region_result
clod_region_read(struct clod_region *region, const int64_t *pos, uint8_t *buff, size_t buff_size, size_t *size);

/**
 * Write chunk data or delete a chunk.
 * Delete a chunk by passing a null buffer.
 * @param[in] region Region handle.
 * @param[in] pos Chunk position.
 * @param[in] buff Buffer containing chunk data to write.
 * @param[in] buff_size Size of the chunk data to write.
 * @throws CLOD_REGION_OK On success.
 * @throws CLOD_REGION_INVALID_USAGE On invalid usage.
 * @throws CLOD_REGION_MALFORMED Chunk data is corrupted.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2)
enum clod_region_result
clod_region_write(struct clod_region *region, const int64_t *pos, const uint8_t *buff, size_t buff_size);

/**
 * Get the last modification time of the chunk.
 * @param[in] region Region handle.
 * @param[in] pos Chunk position.
 * @param[out] mtime Last modification time.
 * @throws CLOD_REGION_OK On success.
 * @throws CLOD_REGION_INVALID_USAGE On invalid usage.
 * @throws CLOD_REGION_MALFORMED Chunk data is corrupted.
 * @throws CLOD_REGION_NOT_FOUND Chunk does not exist.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2)
enum clod_region_result
clod_region_mtime(struct clod_region *region, const int64_t *pos, time_t *mtime);

/**
 * Start iterating over chunks.
 * @param[in] region Region handle.
 * @return New iterator, or nullptr on allocation failure.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1)
struct clod_region_iter *
clod_region_iter_start(struct clod_region *region);

/**
 * Get the next position when iterating over chunks.
 * This method is thread-safe and always returns a unique position.
 * @param[in] iter Iterator.
 * @param[out] pos Next chunk position.
 * @return True if the next position existed and was returned in pos.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2)
bool
clod_region_iter_next(struct clod_region_iter *iter, int64_t *pos);

/**
 * Release resources associated with an iterator.
 * This method is not thread safe.
 * @param[in] iter The iterator to free.
 */
CLOD_API CLOD_NONNULL(1)
void
clod_region_iter_end(struct clod_region_iter *iter);

/**
 * Release resources associated with the region handle.
 * @param r The handle to free.
 */
CLOD_API CLOD_NONNULL(1)
enum clod_region_result
clod_region_close(struct clod_region *r);

/**
 * @name Compression types
 * @{
 * Each algorithm has a unique multiple of ten to represent it,
 * with intermediate values representing the compression level.
 * i.e. compression algorithm 2 at compression level 8 is 28, algorithm 3 at compression level 2 is 32.
 *
 * The provided macros use sane default compression levels for each algorithm,
 * and can be changed using the REGION_COMPRESSION macro.
 * i.e. LZMA at level 9 is REGION_COMPRESSION(REGION_COMPRESS_LZMA, 9).
 */
#define CLOD_REGION_COMPRESSION(algo, level) (((algo)/10 * 10) + ((level) < 0 ? 0 : (level) > 9 ? 9 : (level)))
/** Uncompressed */
#define CLOD_REGION_UNCOMPRESSED 10
#define CLOD_REGION_COMPRESS_GZIP 26
#define CLOD_REGION_COMPRESS_ZLIB 36
#define CLOD_REGION_COMPRESS_LZ4 49
#define CLOD_REGION_COMPRESS_LZ4HC 59
#define CLOD_REGION_COMPRESS_ZSTD 63
#define CLOD_REGION_COMPRESS_LZMA 76
/** @} */

/** @name Open modes
 * @{ */
#define CLOD_REGION_MODE_RDONLY 1
#define CLOD_REGION_MODE_RDWR 2
/** @} */

/** @name Limits
 * @{ */
#define CLOD_REGION_PREFIX_MAX 30
#define CLOD_REGION_EXTENSION_MAX 10
#define CLOD_REGION_DIMENSIONS_MAX 10
#define CLOD_REGION_CHUNK_SIZE_MAX ((size_t)(1 << 40) - 1)
/** @} */

struct clod_region_opts {
	/** Must be the value of the CLOD_REGION_VERSION macro. Used to ensure backwards compatability. */
	uint8_t version;
	/** Number of dimensions. Min 1, Max 10. Defaults to 2. */
	uint8_t dims;
	/** Compression used for new chunks. Defaults to CLOD_REGION_COMPRESS_ZLIB. */
	uint8_t compression;
	/** Open mode. Defaults to CLOD_REGION_MODE_RDWR. */
	uint8_t mode;
	/** Size of region file sectors. This should be sized so that 255 * sector_size
	 * is large enough to hold almost all chunks. Chunks greater than this size are supported,
	 * but do so using dedicated files for each chunk, which is significantly slower. */
	uint32_t sector_size;
	/** File descriptor for the directory relative to which path is resolved.
	 * Allows openat to be used. Can be closed after open.
	 * 0 is reserved as the sentinel nonexistent value. */
	int unix_fd;
	/** File permissions to be applied to newly created files.
	 * Again, 0 is reserved as the sentinel nonexistent value. */
	uint32_t unix_file_perms;
	/** Prefix to filename. Defaults to "region".
	 * Max CLOD_REGION_PREFIX_MAX characters. Must be valid in a filename and cannot contain '.'. */
	char prefix[CLOD_REGION_PREFIX_MAX + 1];
	/** File extension for region files. Defaults to "mcr".
	 * Max CLOD_REGION_EXTENSION_MAX characters. Must be valid in a filename. */
	char region_ext[CLOD_REGION_EXTENSION_MAX + 1];
	/** File extension for chunk files. Defaults to "mcc".
	 * Max CLOD_REGION_EXTENSION_MAX characters. Must be valid in a filename. */
	char chunk_ext[CLOD_REGION_EXTENSION_MAX + 1];
};

/** @} */
#endif
