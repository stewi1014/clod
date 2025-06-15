/**
* @file clod/compression.h
 */
#ifndef CLOD_COMPRESSION_H
#define CLOD_COMPRESSION_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

#define CLOD_COMPRESSION_DEFAULT_LEVEL INT8_MAX

#define CLOD_GZIP (1ull<<0)
#define CLOD_ZLIB (1ull<<1)
#define CLOD_DEFLATE (1ull<<2)
#define CLOD_LZ4 (1ull<<3)
#define CLOD_LZ4HC (1ull<<4)
#define CLOD_LZMA (1ull<<5)
#define CLOD_ZSTD (1ull<<6)

CLOD_API CLOD_CONST
uint64_t
clod_compression_support();

enum clod_compression_result {
	/** The operation was successful. */
	CLOD_COMPRESSION_SUCCESS = 0,
	/** The specified algorithm isn't supported. */
	CLOD_COMPRESSION_UNSUPPORTED = 1,
	/** The compressed data is malformed. */
	CLOD_COMPRESSION_MALFORMED = 2,
	/** The provided buffer is too short to hold the output. */
	CLOD_COMPRESSION_SHORT_BUFFER = 3,
	/** The output buffer is larger than the output, and bytes_written was null. */
	CLOD_COMPRESSION_SHORT_OUTPUT = 4,
	/** Allocation failed. */
	CLOD_COMPRESSION_ALLOC_FAILED = 5
};

struct clod_compressor;
struct clod_decompressor;

/**
 * Create a new compressor.
 * @return Newly allocated compressor, or nullptr on allocation failure.
 */
CLOD_API CLOD_USE_RETURN
struct clod_compressor *
clod_compressor_init();

/**
 * Releases resources associated with the compressor.
 * @param ctx Compressor to free.
 */
CLOD_API CLOD_NONNULL(1)
void
clod_compressor_free(struct clod_compressor *ctx);

/**
 * Compress data with a given compression algorithm.
 * @param[in] ctx Compressor.
 * @param[in] dst Where compressed data is written.
 * @param[in] dst_size Size of dst.
 * @param[in] src Where data to be compressed is read from.
 * @param[in] src_size Size of src.
 * @param[out] compressed_size The actual size of the compressed output,
 * or 0 if compression failed and the compressed size isn't known.
 * @param[in] algo The compression algorith to use.
 * @param[in] level Compression level to use. CLOD_COMPRESSION_DEFAULT_LEVEL will use the algos default.
 * @return Result of compression.
 * @throws CLOD_COMPRESSION_SUCCESS on successful compression.
 * @throws CLOD_COMPRESSION_UNSUPPORTED if the algo is unsupported.
 * @throws CLOD_COMPRESSION_SHORT_BUFFER if the destination buffer is too small to hold the compressed output.
 * @throws CLOD_COMPRESSION_ALLOC_FAILED if memory allocation failed.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2, 4)
enum clod_compression_result
clod_compress(struct clod_compressor *ctx,
	void *dst, size_t dst_size,
	const void *src, size_t src_size,
	size_t *compressed_size,
	uint64_t algo, int8_t level
);

/**
 * Create a new decompressor.
 * @return Newly allocated decompressor, or nullptr on allocation failure.
 */
CLOD_API CLOD_USE_RETURN
struct clod_decompressor *
clod_decompressor_init();

/**
 * Release resources associated with the decompressor.
 * @param ctx Decompressor to free.
 */
CLOD_API CLOD_NONNULL(1)
void
clod_decompressor_free(struct clod_decompressor *ctx);

/**
 * Decompress some data with the given algorithm.
 * @param ctx Decompressor.
 * @param dst Where uncompressed data is written.
 * @param dst_size Size of dst.
 * @param src Where compressed data is read from.
 * @param src_size Size of src.
 * @param decompressed_size The actual size of the uncompressed output,
 * or 0 if decompression failed and the decompressed size isn't known.
 * @param algo The compression algorithm to use.
 * @return The result of the decompression.
 * @throws CLOD_COMPRESSION_SUCCESS on successful decompression.
 * @throws CLOD_COMPRESSION_UNSUPPORTED if the algo is unsupported.
 * @throws CLOD_COMPRESSION_MALFORMED if the compressed data is malformed. THIS IS NOT AN INTEGRITY CHECK.
 * @throws CLOD_COMPRESSION_SHORT_BUFFER if the destination buffer is too small to hold the decompressed output.
 * @throws CLOD_COMPRESSION_SHORT_OUTPUT if dst is larger than the decompressed output and bytes_written is null.
 * @throws CLOD_COMPRESSION_ALLOC_FAILED if memory allocation failed.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2, 4)
enum clod_compression_result
clod_decompress(struct clod_decompressor *ctx,
	void *dst, size_t dst_size,
	const void *src, size_t src_size,
	size_t *decompressed_size,
	uint64_t algo
);

#endif
