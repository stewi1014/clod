#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <libdeflate.h>
#include <errno.h>
#include <string.h>

#include "test.h"
#include <mc.h>

int parse_raw_nbt() {
    FILE *f = fopen("bigtest_raw.nbt", "rb");
    if (f == NULL) {
        printf(strerror(errno));
        return -1;
    }

    char *data = malloc(2048);
    fread(data, 2048, 1, f);
    fclose(f);

    struct MC_nbt nbt = MC_NBT_NEW;
    MC_nbt_parse(&nbt, data, INT_MAX);
    MC_nbt_free(&nbt);

    return 0;
}

/*
int parse_gzip_nbt() {
    FILE *f = fopen("bigtest_gzip.nbt", "rb");
    char *data = malloc(2048);
    fread(data, 2048, 1, f);
    fclose(f);

    char *uncompressed = malloc(2048);
    
    struct libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
    libdeflate_gzip_decompress(decompressor, data, 2048, uncompressed, 2048, NULL);
    libdeflate_free_decompressor(decompressor);

    struct MC_nbt nbt;
    char *end = MC_nbt_parse(&nbt, uncompressed, INT_MAX);

    printf("nbt file %d bytes big\n", (int)(end - uncompressed));
    MC_nbt_free(nbt);

    return 0;
}

int parse_zlib_nbt() {
    return 0;
}
*/
