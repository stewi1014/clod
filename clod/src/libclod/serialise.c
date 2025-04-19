#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zstd.h>
#include <lzma.h>
#include <lz4.h>

#include "clod.h"

#define ZSTD_SIGNATURE "\x28\xb5\x2f\xfd"
#define LZ4_SIGNATURE "\x04\x22\x4d\x18"
enum CLOD_CompressionAlgo guess_compression(char *data) {
    if (strncmp(ZSTD_SIGNATURE, data, strlen(ZSTD_SIGNATURE))) return CLOD_ZSTD;
    if (strncmp(LZ4_SIGNATURE, data, strlen(LZ4_SIGNATURE))) return CLOD_LZ4;
    return CLOD_LZMA;
}

#define BUFFERSIZE (1<<14)

struct CLOD_SerialiseCtx {
    ZSTD_CStream *zstd_stream;
    lzma_stream *lzma_stream;
    LZ4_stream_t *lz4_stream;
};

struct CLOD_SerialiseCtx *CLOD_createSerialiseCtx() {
    struct CLOD_SerialiseCtx *ctx = malloc(sizeof(struct CLOD_SerialiseCtx));
    ctx->zstd_stream = NULL;
    ctx->lzma_stream = NULL;
    ctx->lz4_stream = NULL;
    return ctx;
}

int CLOD_serialiseSection(struct CLOD_SerialiseCtx *ctx, struct CLOD_Section* section, FILE *out, union CLOD_CompressionOptions opts) {
    char buffer[BUFFERSIZE];
    switch (opts.algo) {
        case CLOD_ZSTD:
            break;
        case CLOD_LZMA:
            break;
        case CLOD_LZ4:
            break;
        }
}

void CLOD_freeSerialiseCtx(struct CLOD_SerialiseCtx* ctx) {
    if (ctx->zstd_stream != NULL) ZSTD_freeCStream(ctx->zstd_stream);
    if (ctx->lzma_stream != NULL) (lzma_end(ctx->lzma_stream), free(ctx->lzma_stream));
    if (ctx->lz4_stream != NULL) LZ4_freeStream(ctx->lz4_stream);
    free(ctx);
}

struct CLOD_DeserialiseCtx {
    ZSTD_DStream *zstd_stream;
    lzma_stream *lzma_stream;
    LZ4_streamDecode_t *lz4_stream;
};

struct CLOD_DeserialiseCtx *CLOD_createDeserialiseCtx() {
    struct CLOD_DeserialiseCtx *ctx = malloc(sizeof(struct CLOD_DeserialiseCtx));
    ctx->zstd_stream = NULL;
    ctx->lzma_stream = NULL;
    ctx->lz4_stream = NULL;
    return ctx;
}

int CLOD_deserialiseSection(struct CLOD_DeserialiseCtx *ctx, struct CLOD_Section* section, FILE *in){
    char buffer[BUFFERSIZE];
    size_t bytes_read;
    int error;
    
    bytes_read = fread(buffer, sizeof(buffer), 1, in);
    if (error = ferror(in) != 0) return error;
    
    enum CLOD_CompressionAlgo algo = guess_compression(buffer);
    switch (algo) {
    case CLOD_ZSTD:
        break;
    case CLOD_LZMA:
        break;
    case CLOD_LZ4:
        break;
    }
}

void CLOD_freeDeserialiseCtx(struct CLOD_DeserialiseCtx *ctx){
    if (ctx->zstd_stream != NULL) ZSTD_freeDStream(ctx->zstd_stream);
    if (ctx->lzma_stream != NULL) (lzma_end(ctx->lzma_stream), free(ctx->lzma_stream));
    if (ctx->lz4_stream != NULL) LZ4_freeStreamDecode(ctx->lz4_stream);
    free(ctx);
}
