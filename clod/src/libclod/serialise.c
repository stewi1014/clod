#include <stdio.h>
#include <stdlib.h>

#include <zstd.h>
#include <lzma.h>

#include "clod.h"

#define ZSTD_SIGNATURE '\x28\xb5\x2f\xfd'
#define LZ4_SIGNATURE '\x04\x22\x4d\x18'

struct CLOD_SerialiseCtx {
    ZSTD_CStream *zstd_stream;
    lzma_stream *lzma_stream;
};

struct CLOD_SerialiseCtx *CLOD_createSerialiseCtx() {
    struct CLOD_SerialiseCtx *ctx = malloc(sizeof(struct CLOD_SerialiseCtx));
    ctx->zstd_stream = NULL;
    ctx->lzma_stream = NULL;
    return ctx;
}

void CLOD_serialiseSection(struct CLOD_SerialiseCtx *ctx, struct CLOD_Section* section, FILE out) {

}

void CLOD_freeSerialiseCtx(struct CLOD_SerialiseCtx* ctx) {
    if (ctx->zstd_stream != NULL) ZSTD_freeCStream(ctx->zstd_stream);
    if (ctx->lzma_stream != NULL) (lzma_end(ctx->lzma_stream), free(ctx->lzma_stream));
    free(ctx);
}



struct CLOD_DeserialiseCtx {
    ZSTD_DStream *zstd_stream;
    lzma_stream *lzma_stream;
};

struct CLOD_DeserialiseCtx *CLOD_createDeserialiseCtx() {
    struct CLOD_DeserialiseCtx *ctx = malloc(sizeof(struct CLOD_DeserialiseCtx));
    ctx->zstd_stream = NULL;
    ctx->lzma_stream = NULL;
    return ctx;
}

void CLOD_deserialiseSection(struct CLOD_DeserialiseCtx *ctx, struct CLOD_Section* section, FILE in){

}

void CLOD_freeDeserialiseCtx(struct CLOD_DeserialiseCtx *ctx){
    if (ctx->zstd_stream != NULL) ZSTD_freeDStream(ctx->zstd_stream);
    if (ctx->lzma_stream != NULL) (lzma_end(ctx->lzma_stream), free(ctx->lzma_stream));
    free(ctx);
}
