#include <clod/compression.h>
#include "compression_config.h"

uint64_t clod_compression_support() {
	uint64_t ret = 0;

#if HAVE_LIBDEFLATE
	ret |= CLOD_DEFLATE | CLOD_GZIP | CLOD_ZLIB;
#endif

#if HAVE_LZ4
	ret |= CLOD_LZ4 | CLOD_LZ4HC;
#endif

#if HAVE_LZMA
	ret |= CLOD_LZMA;
#endif

#if HAVE_ZSTD
	ret |= CLOD_ZSTD;
#endif

	return ret;
}
