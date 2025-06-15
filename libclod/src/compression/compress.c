#include <clod/compression.h>
#include <stdlib.h>
#include <string.h>

#include "compression_libraries.h"

struct clod_compressor {
#if HAVE_LIBDEFLATE
	struct libdeflate_compressor *libdeflate_compressor;
	int libdeflate_compressor_level;
#endif

#if HAVE_LZ4
	LZ4_stream_t *lz4_stream;
	LZ4_streamHC_t *lz4hc_stream;
#endif

#if HAVE_LZMA
	lzma_stream *lzma_stream;
#endif

#if HAVE_ZSTD
	ZSTD_CCtx *zstd_compressor;
#endif
};

struct clod_compressor *clod_compressor_init() {
	struct clod_compressor *ctx = malloc(sizeof(struct clod_compressor));
	memset(ctx, 0, sizeof(*ctx));
	return ctx;
}

void clod_compressor_free(struct clod_compressor *ctx) {
#if HAVE_LIBDEFLATE
	if (ctx->libdeflate_compressor)
		libdeflate_free_compressor(ctx->libdeflate_compressor);
#endif

#if HAVE_LZ4
	if (ctx->lz4_stream)
		LZ4_freeStream(ctx->lz4_stream);
	if (ctx->lz4hc_stream)
		LZ4_freeStreamHC(ctx->lz4hc_stream);
#endif

#if HAVE_LZMA
	if (ctx->lzma_stream) {
		lzma_end(ctx->lzma_stream);
		free(ctx->lzma_stream);
	}
#endif

#if HAVE_ZSTD
	if (ctx->zstd_compressor)
		ZSTD_freeCCtx(ctx->zstd_compressor);
#endif

	free(ctx);
}

enum clod_compression_result
clod_compress(struct clod_compressor *ctx,
	void *dst, const size_t dst_size,
	const void *src, const size_t src_size,
	size_t *compressed_size,
	const uint64_t algo, int8_t level
) {
	switch (algo) {
		case CLOD_GZIP: {
#if HAVE_LIBDEFLATE
			if (level == CLOD_COMPRESSION_DEFAULT_LEVEL) level = 6;

			if (ctx->libdeflate_compressor && ctx->libdeflate_compressor_level != level) {
				libdeflate_free_compressor(ctx->libdeflate_compressor);
				ctx->libdeflate_compressor = nullptr;
			}
			if (!ctx->libdeflate_compressor) {
				ctx->libdeflate_compressor = libdeflate_alloc_compressor(level);
				if (!ctx->libdeflate_compressor) {
					return CLOD_COMPRESSION_ALLOC_FAILED;
				}
				ctx->libdeflate_compressor_level = level;
			}

			const size_t size = libdeflate_gzip_compress(ctx->libdeflate_compressor,
				src, src_size,
				dst, dst_size);

			if (compressed_size)
				*compressed_size = size;
			else if (size != dst_size)
				return CLOD_COMPRESSION_SHORT_OUTPUT;

			return CLOD_COMPRESSION_SUCCESS;
#else
			return CLOD_COMPRESSION_UNSUPPORTED;
#endif
		}
		case CLOD_ZLIB: {
#if HAVE_LIBDEFLATE
			if (level == CLOD_COMPRESSION_DEFAULT_LEVEL) level = 6;

			if (ctx->libdeflate_compressor && ctx->libdeflate_compressor_level != level) {
				libdeflate_free_compressor(ctx->libdeflate_compressor);
				ctx->libdeflate_compressor = nullptr;
			}
			if (!ctx->libdeflate_compressor) {
				ctx->libdeflate_compressor = libdeflate_alloc_compressor(level);
				if (!ctx->libdeflate_compressor) {
					return CLOD_COMPRESSION_ALLOC_FAILED;
				}
				ctx->libdeflate_compressor_level = level;
			}

			const size_t size = libdeflate_zlib_compress(ctx->libdeflate_compressor,
				src, src_size,
				dst, dst_size);

			if (compressed_size)
				*compressed_size = size;
			else if (size != dst_size)
				return CLOD_COMPRESSION_SHORT_OUTPUT;

			return CLOD_COMPRESSION_SUCCESS;
#else
			return CLOD_COMPRESSION_UNSUPPORTED;
#endif
		}
		case CLOD_DEFLATE: {
#if HAVE_LIBDEFLATE
			if (level == CLOD_COMPRESSION_DEFAULT_LEVEL) level = 6;

			if (ctx->libdeflate_compressor && ctx->libdeflate_compressor_level != level) {
				libdeflate_free_compressor(ctx->libdeflate_compressor);
				ctx->libdeflate_compressor = nullptr;
			}

			if (!ctx->libdeflate_compressor) {
				ctx->libdeflate_compressor = libdeflate_alloc_compressor(level);
				if (!ctx->libdeflate_compressor) {
					return CLOD_COMPRESSION_ALLOC_FAILED;
				}
				ctx->libdeflate_compressor_level = level;
			}

			const size_t size = libdeflate_deflate_compress(ctx->libdeflate_compressor,
				src, src_size,
				dst, dst_size);

			if (compressed_size)
				*compressed_size = size;
			else if (size != dst_size)
				return CLOD_COMPRESSION_SHORT_OUTPUT;

			return CLOD_COMPRESSION_SUCCESS;
#else
			return CLOD_COMPRESSION_UNSUPPORTED;
#endif
		}
		case CLOD_LZ4: {
			return CLOD_COMPRESSION_UNSUPPORTED;
		}
		case CLOD_LZ4HC: {
			return CLOD_COMPRESSION_UNSUPPORTED;
		}
		case CLOD_LZMA: {
			return CLOD_COMPRESSION_UNSUPPORTED;
		}
		case CLOD_ZSTD: {
			return CLOD_COMPRESSION_UNSUPPORTED;
		}
		default: {
			return CLOD_COMPRESSION_UNSUPPORTED;
		}
	}
}
