#include <clod/compression.h>
#include <stdlib.h>
#include <string.h>

#include "compression_libraries.h"

struct clod_decompressor {
#if HAVE_LIBDEFLATE
	struct libdeflate_decompressor *libdeflate_decompressor;
#endif

#if HAVE_LZ4
	LZ4_streamDecode_t *lz4_streamDecode;
	LZ4_streamHC_t *lz4hc_streamDecode;
#endif

#if HAVE_LZMA
	lzma_stream *lzma_stream;
#endif

#if HAVE_ZSTD
	ZSTD_DCtx *zstd_decompressor;
#endif
};

struct clod_decompressor *clod_decompressor_init() {
	struct clod_decompressor *ctx = malloc(sizeof(struct clod_decompressor));
	memset(ctx, 0,sizeof(*ctx));
	return ctx;
}

void clod_decompressor_free(struct clod_decompressor *ctx) {
#if HAVE_LIBDEFLATE
	if (ctx->libdeflate_decompressor)
		libdeflate_free_decompressor(ctx->libdeflate_decompressor);
#endif

#if HAVE_LZ4
	if (ctx->lz4_streamDecode)
		LZ4_freeStreamDecode(ctx->lz4_streamDecode);
	if (ctx->lz4hc_streamDecode)
		LZ4_freeStreamHC(ctx->lz4hc_streamDecode);
#endif

#if HAVE_LZMA
	if (ctx->lzma_stream) {
		lzma_end(ctx->lzma_stream);
		free(ctx->lzma_stream);
	}
#endif

#if HAVE_ZSTD
	if (ctx->zstd_decompressor)
		ZSTD_freeDCtx(ctx->zstd_decompressor);
#endif

	free(ctx);
}

enum clod_compression_result
clod_decompress(struct clod_decompressor *ctx,
	void *dst, const size_t dst_size,
	const void *src, const size_t src_size,
	size_t *decompressed_size,
	const uint64_t algo
) {
	switch (algo) {
		case CLOD_GZIP: {
#if HAVE_LIBDEFLATE
			if (!ctx->libdeflate_decompressor) {
				ctx->libdeflate_decompressor = libdeflate_alloc_decompressor();
				if (!ctx->libdeflate_decompressor) {
					return CLOD_COMPRESSION_ALLOC_FAILED;
				}
			}

			const enum libdeflate_result res = libdeflate_gzip_decompress(ctx->libdeflate_decompressor,
				src, src_size,
				dst, dst_size,
				decompressed_size
			);

			switch (res) {
				case LIBDEFLATE_SUCCESS: return CLOD_COMPRESSION_SUCCESS;
				case LIBDEFLATE_BAD_DATA: return CLOD_COMPRESSION_MALFORMED;
				case LIBDEFLATE_SHORT_OUTPUT: return CLOD_COMPRESSION_SHORT_OUTPUT;
				case LIBDEFLATE_INSUFFICIENT_SPACE: return CLOD_COMPRESSION_SHORT_BUFFER;
				default: return CLOD_COMPRESSION_UNSUPPORTED;
			}
#else
			return CLOD_COMPRESSION_UNSUPPORTED;
#endif
		}
		case CLOD_ZLIB: {
#if HAVE_LIBDEFLATE
			if (!ctx->libdeflate_decompressor) {
				ctx->libdeflate_decompressor = libdeflate_alloc_decompressor();
				if (!ctx->libdeflate_decompressor) {
					return CLOD_COMPRESSION_ALLOC_FAILED;
				}
			}

			const enum libdeflate_result res = libdeflate_zlib_decompress(ctx->libdeflate_decompressor,
				src, src_size,
				dst, dst_size,
				decompressed_size
			);

			switch (res) {
				case LIBDEFLATE_SUCCESS: return CLOD_COMPRESSION_SUCCESS;
				case LIBDEFLATE_BAD_DATA: return CLOD_COMPRESSION_MALFORMED;
				case LIBDEFLATE_SHORT_OUTPUT: return CLOD_COMPRESSION_SHORT_OUTPUT;
				case LIBDEFLATE_INSUFFICIENT_SPACE: return CLOD_COMPRESSION_SHORT_BUFFER;
				default: return CLOD_COMPRESSION_UNSUPPORTED;
			}
#else
			return CLOD_COMPRESSION_UNSUPPORTED;
#endif
		}
		case CLOD_DEFLATE: {
#if HAVE_LIBDEFLATE
			if (!ctx->libdeflate_decompressor) {
				ctx->libdeflate_decompressor = libdeflate_alloc_decompressor();
				if (!ctx->libdeflate_decompressor) {
					return CLOD_COMPRESSION_ALLOC_FAILED;
				}
			}

			const enum libdeflate_result res = libdeflate_deflate_decompress(ctx->libdeflate_decompressor,
				src, src_size,
				dst, dst_size,
				decompressed_size
			);

			switch (res) {
				case LIBDEFLATE_SUCCESS: return CLOD_COMPRESSION_SUCCESS;
				case LIBDEFLATE_BAD_DATA: return CLOD_COMPRESSION_MALFORMED;
				case LIBDEFLATE_SHORT_OUTPUT: return CLOD_COMPRESSION_SHORT_OUTPUT;
				case LIBDEFLATE_INSUFFICIENT_SPACE: return CLOD_COMPRESSION_SHORT_BUFFER;
				default: return CLOD_COMPRESSION_UNSUPPORTED;
			}
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
