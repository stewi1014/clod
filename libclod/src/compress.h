#pragma once

#include <stddef.h>
#include <lzma.h>

int compress_lz4(
    void **ctx,
    char *in,
    size_t in_len,
    char **out,
    size_t *out_len,
    size_t *actual_out,
    void *(*realloc_f)(void*, size_t)
);

void compress_free_lz4(
    void **ctx,
    void *(*realloc_f)(void*, size_t)
);

lzma_ret compress_lzma(
    void **ctx,
    char *in,
    size_t in_len,
    char **out,
    size_t *out_cap,
    size_t *actual_out,
    void *(*realloc_f)(void*, size_t)
);

void compress_free_lzma(
    void **ctx,
    void *(*realloc_f)(void*, size_t)
);
