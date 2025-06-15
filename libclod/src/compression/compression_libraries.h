#ifndef COMPRESSION_LIBRARIES_H
#define COMPRESSION_LIBRARIES_H

#include <clod/compression.h>
#include "compression_config.h"

#if HAVE_LIBDEFLATE
#include <libdeflate.h>
#endif

#if HAVE_LZ4
#include <lz4.h>
#include <lz4hc.h>
#endif

#if HAVE_LZMA
#include <lzma.h>
#endif

#if HAVE_ZSTD
#include <zstd.h>
#endif

#endif
