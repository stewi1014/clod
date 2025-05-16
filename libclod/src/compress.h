#pragma once

#include <stddef.h>
#include <lzma.h>

int compress_lz4(
    void **ctx_ptr,
    const char *in,
    size_t in_len,
    char **out,
    size_t *out_cap,
    size_t *actual_out,
    void *(*realloc_f)(void*, size_t),
    double level
);

void compress_free_lz4(
    void **ctx_ptr,
    void *(*realloc_f)(void*, size_t)
);

lzma_ret compress_lzma(
    void **ctx_ptr,
    const char *in,
    size_t in_len,
    char **out,
    size_t *out_cap,
    size_t *actual_out,
    void *(*realloc_f)(void*, size_t),
    double level
);

void compress_free_lzma(
    void **ctx_ptr,
    void *(*realloc_f)(void*, size_t)
);
