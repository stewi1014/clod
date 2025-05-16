#include <lzma.h>
#include <lz4frame.h>

#include "compress.h"

#define BUFFER_GROW(cap) ((cap == 0) ? 128 * 1024 : (cap << 1) - (cap >> 1))

int compress_lz4(
    void **ctx_ptr,
    const char *in,
    const size_t in_len,
    char **out,
    size_t *out_cap,
    size_t *actual_out,
    void *(*realloc_f)(void*, size_t)
) {
    const auto ctx = (LZ4F_cctx**)ctx_ptr;

    LZ4F_preferences_t prefs = {0};
    prefs.compressionLevel = 2;
    prefs.frameInfo.blockSizeID = LZ4F_max64KB;
    prefs.frameInfo.blockMode = LZ4F_blockIndependent;

    if (*ctx == nullptr) {
        const LZ4F_errorCode_t err = LZ4F_createCompressionContext(ctx, LZ4F_VERSION);
        if (LZ4F_isError(err)) {
            return -1;
        }
    }

    const size_t max_compressed_size = LZ4F_HEADER_SIZE_MAX + LZ4F_compressBound(in_len, &prefs);
    if (*out_cap < max_compressed_size) {
        char *new = realloc_f(*out, max_compressed_size);
        if (new == nullptr) {
            return -1;
        }

        *out = new;
        *out_cap = max_compressed_size;
    }

    const size_t header_size = LZ4F_compressBegin(*ctx, *out, *out_cap, &prefs);
    if (LZ4F_isError(header_size)) {
        return -1;
    }

    const size_t compressed_size = LZ4F_compressUpdate(
        *ctx,
        *out + header_size,
        *out_cap - header_size,
        in,
        in_len,
        nullptr
    );
    if (LZ4F_isError(compressed_size)) {
        return -1;
    }

    const size_t end_size = LZ4F_compressEnd(
        *ctx, 
        *out + header_size + compressed_size, 
        *out_cap - header_size - compressed_size,
        nullptr
    );
    if (LZ4F_isError(end_size)) {
        return -1;
    }

    *actual_out = header_size + compressed_size + end_size;
    return 0;
}

void compress_free_lz4(
    void **ctx_ptr,
    void *(*realloc_f)(void*, size_t)
) {
    const auto ctx = (LZ4F_cctx**)ctx_ptr;

    if (*ctx != nullptr) {
        LZ4F_freeCompressionContext(*ctx);
        *ctx = nullptr;
    }
}

lzma_ret compress_lzma(
    void **ctx_ptr,
    const char *in,
    const size_t in_len,
    char **out,
    size_t *out_cap,
    size_t *actual_out,
    void *(*realloc_f)(void*, size_t)
) {
    lzma_stream *strm = *ctx_ptr;
    lzma_ret result;

    if (strm == nullptr) {
        strm = realloc_f(nullptr, sizeof(lzma_stream));
        if (strm == nullptr) {
            return LZMA_MEM_ERROR;
        }

        *strm = (lzma_stream)LZMA_STREAM_INIT;
        *ctx_ptr = strm;

        result = lzma_easy_encoder(strm, 0, LZMA_CHECK_CRC32);
        if (result != LZMA_OK) {
            return result;
        }
    }

    strm->next_in = (uint8_t*)in;
    strm->avail_in = in_len;
    strm->total_in = 0;

    strm->next_out = (uint8_t*)*out;
    strm->avail_out = *out_cap;
    strm->total_out = 0;

    do {
        if (strm->avail_out == 0) {
            const size_t new_cap = BUFFER_GROW(*out_cap);
            char *new = realloc_f(*out, new_cap);
            if (new == nullptr) {
                return LZMA_MEM_ERROR;
            }

            *out = new;
            *out_cap = new_cap;
            
            strm->next_out = (uint8_t*)new + strm->total_out;
            strm->avail_out = new_cap - strm->total_out;
        }

        result = lzma_code(strm, LZMA_FINISH);
    } while (result == LZMA_OK && strm->avail_in > 0);

    *actual_out = strm->total_out;
    return result;
}

void compress_free_lzma(
    void **ctx_ptr,
    void *(*realloc_f)(void*, size_t)
) {
    lzma_stream *strm = *ctx_ptr;
    if (strm != nullptr) {
        lzma_end(strm);
        realloc_f(strm, 0);
        *ctx_ptr = nullptr;
    }
}
