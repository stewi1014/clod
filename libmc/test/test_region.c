#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <libdeflate.h>

#include "test.h"
#include <mc.h>

int open_region() {
    FILE *f = fopen("r.-3.1.mca", "rb");
    if (f == NULL) {
        printf(strerror(errno));
        return -1;
    }

    char *data = malloc(12136448); //uuh
    int read = fread(data, 12136448, 1, f);
    assert(read == 1);
    fclose(f);

    int compressed_total = 0, uncompressed_total = 0;

    struct libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
    char *decompressed = malloc(128 * 1024);

    struct MC_nbt nbt = MC_NBT_NEW;

    for (int x = 0; x < 32; x++) for (int z = 0; z < 32; z++) {
        char *chunk_data = MC_region_get_chunk(data, x, z);

        if (MC_chunk_compression(chunk_data) != MC_CHUNK_COMPRESSION_ZLIB) return -1;

        enum libdeflate_result result = libdeflate_zlib_decompress(
            decompressor, 
            MC_chunk_nbt_data(chunk_data), 
            MC_chunk_length(chunk_data),
            decompressed,
            10 * 1024 * 1024,
            NULL
        );

        assert(result == LIBDEFLATE_SUCCESS || result == LIBDEFLATE_SHORT_OUTPUT);

        char *end;

        end = MC_nbt_parse(&nbt, decompressed, INT32_MAX);
        compressed_total += MC_chunk_length(chunk_data);
        uncompressed_total += (int)(end - decompressed);

    }
    
    libdeflate_free_decompressor(decompressor);
    
    MC_nbt_free(&nbt);
    free(decompressed);
    free(data);
    return 0;
}

int open_region_nokeep() {
    FILE *f = fopen("r.-3.1.mca", "rb");
    if (f == NULL) {
        printf(strerror(errno));
        return -1;
    }

    char *data = malloc(12136448); //uuh
    int read = fread(data, 12136448, 1, f);
    assert(read == 1);
    fclose(f);

    int compressed_total = 0, uncompressed_total = 0;

    struct libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
    char *decompressed = malloc(10 * 1024 * 1024);

    struct MC_nbt nbt = MC_NBT_NEW;

    for (int x = 0; x < 32; x++) for (int z = 0; z < 32; z++) {
        char *chunk_data = MC_region_get_chunk(data, x, z);

        if (MC_chunk_compression(chunk_data) != MC_CHUNK_COMPRESSION_ZLIB) return -1;

        enum libdeflate_result result = libdeflate_zlib_decompress(
            decompressor, 
            MC_chunk_nbt_data(chunk_data), 
            MC_chunk_length(chunk_data),
            decompressed,
            10 * 1024 * 1024,
            NULL
        );

        assert(result == LIBDEFLATE_SUCCESS || result == LIBDEFLATE_SHORT_OUTPUT);

        char *end;

        end = MC_nbt_parse(NULL, decompressed, INT32_MAX);
        compressed_total += MC_chunk_length(chunk_data);
        uncompressed_total += (int)(end - decompressed);
    }

    libdeflate_free_decompressor(decompressor);

    MC_nbt_free(&nbt);
    free(decompressed);
    free(data);
    return 0;
}

