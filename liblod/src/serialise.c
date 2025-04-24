#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zstd.h>
#include <lzma.h>
#include <lz4.h>

#include <lod.h>

#define ZSTD_SIGNATURE "\x28\xb5\x2f\xfd"
#define LZ4_SIGNATURE "\x04\x22\x4d\x18"
enum LOD_CompressionAlgo guess_compression(char *data) {
    if (strncmp(ZSTD_SIGNATURE, data, strlen(ZSTD_SIGNATURE))) return LOD_ZSTD;
    if (strncmp(LZ4_SIGNATURE, data, strlen(LZ4_SIGNATURE))) return LOD_LZ4;
    return LOD_LZMA;
}

#define BUFFERSIZE (1<<14)

struct LOD_SerialiseCtx {
    ZSTD_CStream *zstd_stream;
    lzma_stream *lzma_stream;
    LZ4_stream_t *lz4_stream;
};

struct LOD_SerialiseCtx *LOD_createSerialiseCtx() {
    struct LOD_SerialiseCtx *ctx = malloc(sizeof(struct LOD_SerialiseCtx));
    ctx->zstd_stream = NULL;
    ctx->lzma_stream = NULL;
    ctx->lz4_stream = NULL;
    return ctx;
}

int LOD_serialiseSection(struct LOD_SerialiseCtx *ctx, struct LOD_Section* section, FILE *out, union LOD_CompressionOptions opts);

void LOD_freeSerialiseCtx(struct LOD_SerialiseCtx* ctx) {
    if (ctx->zstd_stream != NULL) ZSTD_freeCStream(ctx->zstd_stream);
    if (ctx->lzma_stream != NULL) (lzma_end(ctx->lzma_stream), free(ctx->lzma_stream));
    if (ctx->lz4_stream != NULL) LZ4_freeStream(ctx->lz4_stream);
    free(ctx);
}

struct LOD_DeserialiseCtx {
    ZSTD_DStream *zstd_stream;
    lzma_stream *lzma_stream;
    LZ4_streamDecode_t *lz4_stream;
};

struct LOD_DeserialiseCtx *LOD_createDeserialiseCtx() {
    struct LOD_DeserialiseCtx *ctx = malloc(sizeof(struct LOD_DeserialiseCtx));
    ctx->zstd_stream = NULL;
    ctx->lzma_stream = NULL;
    ctx->lz4_stream = NULL;
    return ctx;
}

int LOD_deserialiseSection(struct LOD_DeserialiseCtx *ctx, struct LOD_Section* section, FILE *in);

void LOD_freeDeserialiseCtx(struct LOD_DeserialiseCtx *ctx){
    if (ctx->zstd_stream != NULL) ZSTD_freeDStream(ctx->zstd_stream);
    if (ctx->lzma_stream != NULL) (lzma_end(ctx->lzma_stream), free(ctx->lzma_stream));
    if (ctx->lz4_stream != NULL) LZ4_freeStreamDecode(ctx->lz4_stream);
    free(ctx);
}
