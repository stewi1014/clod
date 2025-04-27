#include <stdlib.h>
#include <stdbool.h>

#include <libdeflate.h>
#include <lz4.h>

#include "region.h"

size_t decompress_deflate(struct region_reader *reader, char *chunk_data, char type);

struct region_reader {
    char *chunk_data;
    struct libdeflate_decompressor *decompressor;
    size_t capacity;
};

struct region_reader *region_reader_alloc() {
    struct region_reader *reader = malloc(sizeof(struct region_reader));
    if (reader == NULL) return NULL;

    reader->decompressor = NULL;
    reader->capacity = 128 * 1024;
    reader->chunk_data = malloc(reader->capacity);

    if (reader->chunk_data == NULL) {
        free(reader);
        return NULL;
    }

    return reader;
}

void region_reader_free(struct region_reader *reader) {
    if (reader == NULL) return;
    
    if (reader->decompressor != NULL) {
        libdeflate_free_decompressor(reader->decompressor);
        reader->decompressor = NULL;
    }

    if (reader->chunk_data != NULL) {
        free(reader->chunk_data);
        reader->chunk_data = NULL;
    }

    return;
}

#define REGION_COMPRESSION_GZIP 1
#define REGION_COMPRESSION_ZLIB 2
#define REGION_COMPRESSION_UNCOMPRESSED 3
#define REGION_COMPRESSION_LZ4 4
#define REGION_COMPRESSION_CUSTOM 127

char *region_read_chunk(struct region_reader *reader, char *region, int chunk_x, int chunk_z) {
    if (reader == NULL) return NULL;
    if (region == NULL) return NULL;

    int chunk_index = ((chunk_x & 31) + (chunk_z & 31) * 32);
    int offset = 
        ((int)(unsigned char)region[0 + 4 * chunk_index] << 16) +
        ((int)(unsigned char)region[1 + 4 * chunk_index] << 8) +
        ((int)(unsigned char)region[2 + 4 * chunk_index]);
    unsigned char sectors =
        region[3 + 4 * chunk_index];

    if (offset < 2 || sectors == 0) return NULL;

    char *chunk_data = region + 4096 * offset;

    switch (chunk_data[4]) {
    case REGION_COMPRESSION_GZIP: 
    case REGION_COMPRESSION_ZLIB: {
        size_t decompressed_size = decompress_deflate(reader, chunk_data, chunk_data[4]);
        if (decompressed_size == 0) {
            return NULL;
        }
        return reader->chunk_data;
    }
    case REGION_COMPRESSION_UNCOMPRESSED: return &chunk_data[5]; //lol
    case REGION_COMPRESSION_LZ4: {
        return NULL;
    }
    case REGION_COMPRESSION_CUSTOM: {
        return NULL;
    }
    }

    return NULL;
}

size_t decompress_deflate(struct region_reader *reader, char *chunk_data, char type) {
    int chunk_size =
        ((int)chunk_data[0] << 24) |
        ((int)chunk_data[1] << 16) |
        ((int)chunk_data[2] << 8)  |
        ((int)chunk_data[3]);

    if (reader->decompressor == NULL) reader->decompressor = libdeflate_alloc_decompressor();

    enum libdeflate_result result;
    size_t decompressed_size;

try_again:
    if (type == REGION_COMPRESSION_GZIP) {
        result = libdeflate_gzip_decompress(
            reader->decompressor, 
            &chunk_data[5], 
            chunk_size,
            reader->chunk_data,
            reader->capacity,
            &decompressed_size
        );
    } else if (type == REGION_COMPRESSION_ZLIB) {
        result = libdeflate_zlib_decompress(
            reader->decompressor, 
            &chunk_data[5], 
            chunk_size,
            reader->chunk_data,
            reader->capacity,
            &decompressed_size
        );
    } else {
        return 0;
    }

    if (result == LIBDEFLATE_SUCCESS || result == LIBDEFLATE_SHORT_OUTPUT)
        return decompressed_size;

    if (result == LIBDEFLATE_INSUFFICIENT_SPACE){
        size_t new_capacity = reader->capacity * 2;
        char *new_buffer = realloc(reader->chunk_data, new_capacity);

        if (new_buffer == NULL) return 0;

        reader->chunk_data = new_buffer;
        reader->capacity = new_capacity;
        goto try_again;
    }

    return 0;
}
