#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <libdeflate.h>

#include "nbt.h"
#include "anvil.h"
#include "anvil_world.h"

#define BUFFER_START 100 * 1024
#define BUFFER_MAX 4 * 1024 * 1024



//=======//
// Chunk //
//=======//

struct anvil_chunk_ctx {
    char *buffer;
    size_t buffer_cap;

    struct libdeflate_decompressor *libdeflate_decompressor;

    void *(*malloc)(size_t);
    void *(*realloc)(void*, size_t);
    void (*free)(void*);
};

struct anvil_chunk_ctx *anvil_chunk_ctx_alloc(struct anvil_world *world) {
    struct anvil_chunk_ctx *ctx;
    if (world == NULL) {
        ctx = malloc(sizeof(struct anvil_chunk_ctx));
        if (ctx == NULL) return NULL;
        ctx->malloc = malloc;
        ctx->realloc = realloc;
        ctx->free = free;
    } else {
        ctx = world->malloc(sizeof(struct anvil_chunk_ctx));
        if (ctx == NULL) return NULL;
        ctx->malloc = world->malloc;
        ctx->realloc = world->realloc;
        ctx->free = world->free;
    }

    ctx->libdeflate_decompressor = NULL;
    ctx->buffer_cap = BUFFER_START;
    ctx->buffer = ctx->malloc(ctx->buffer_cap);
    if (ctx->buffer == NULL) {
        ctx->free(ctx);
        return NULL;
    }

    return ctx;
}

void anvil_chunk_ctx_free(struct anvil_chunk_ctx *ctx) {
    if (ctx->libdeflate_decompressor != NULL) {
        libdeflate_free_decompressor(ctx->libdeflate_decompressor);
    }

    if (ctx->buffer != NULL) {
        ctx->free(ctx->buffer);
    }

    ctx->free(ctx);
}

static struct libdeflate_decompressor *chunk_ctx_libdeflate_decompressor(struct anvil_chunk_ctx *ctx) {
    if (ctx->libdeflate_decompressor == NULL) {
        struct libdeflate_options options;
        memset(&options, 0, sizeof(options));
        options.sizeof_options = sizeof(options);

        options.malloc_func = ctx->malloc;
        options.free_func = ctx->free;

        ctx->libdeflate_decompressor = libdeflate_alloc_decompressor_ex(&options);
    }
    return ctx->libdeflate_decompressor;
}

#define REGION_COMPRESSION_GZIP 1
#define REGION_COMPRESSION_ZLIB 2
#define REGION_COMPRESSION_UNCOMPRESSED 3
#define REGION_COMPRESSION_LZ4 4
#define REGION_COMPRESSION_CUSTOM 127

struct anvil_chunk anvil_chunk_decompress(
    struct anvil_chunk_ctx *ctx,
    struct anvil_region *region,
    int chunk_x,
    int chunk_z
) {
    chunk_x &= 31;
    chunk_z &= 31;

    struct anvil_chunk chunk;
    chunk.data = NULL;
    chunk.data_size = 0;
    chunk.chunk_x = chunk_x;
    chunk.chunk_z = chunk_z;
    if (region->data_size == 0) {
        chunk.data = "";
        return chunk;
    } else if (region->data_size < 8192) {
        return chunk;
    }

    int chunk_index = chunk_x + chunk_z * 32;
    int offset = 
        ((int)(unsigned char)region->data[0 + 4 * chunk_index] << 16) +
        ((int)(unsigned char)region->data[1 + 4 * chunk_index] << 8) +
        ((int)(unsigned char)region->data[2 + 4 * chunk_index]);
    unsigned char sectors =
        region->data[3 + 4 * chunk_index];

    if (offset < 2 || sectors == 0) {
        chunk.data = "";
        return chunk;
    }

    char *chunk_data = region->data + 4096 * offset;

    int compressed_size =
        ((int)(unsigned char)chunk_data[0] << 24) +
        ((int)(unsigned char)chunk_data[1] << 16) +
        ((int)(unsigned char)chunk_data[2] << 8)  +
        ((int)(unsigned char)chunk_data[3]) - 1;

    switch (chunk_data[4]) {
    case REGION_COMPRESSION_GZIP:
    case REGION_COMPRESSION_ZLIB: {
        struct libdeflate_decompressor *decompressor = chunk_ctx_libdeflate_decompressor(ctx);
        size_t decompressed_size;
        enum libdeflate_result result;

    try_deflate_again:
        if (chunk_data[4] == REGION_COMPRESSION_GZIP) {
            result = libdeflate_gzip_decompress(
                decompressor, 
                &chunk_data[5], 
                compressed_size,
                ctx->buffer,
                ctx->buffer_cap,
                &decompressed_size
            );
        } else if (chunk_data[4] == REGION_COMPRESSION_ZLIB) {
            result = libdeflate_zlib_decompress(
                decompressor, 
                &chunk_data[5], 
                compressed_size,
                ctx->buffer,
                ctx->buffer_cap,
                &decompressed_size
            );
        } else {
            assert(0);
        }

        if (result == LIBDEFLATE_INSUFFICIENT_SPACE){
            size_t new_capacity = ctx->buffer_cap * 2;
            char *new_buffer = realloc(ctx->buffer, new_capacity);
    
            if (new_buffer != NULL) {
                ctx->buffer = new_buffer;
                ctx->buffer_cap = new_capacity;
                goto try_deflate_again;
            }
        }

        if (result == LIBDEFLATE_SUCCESS || result == LIBDEFLATE_SHORT_OUTPUT) {
            chunk.data = ctx->buffer;
            chunk.data_size = decompressed_size;
        }

        return chunk;
    };
    case REGION_COMPRESSION_UNCOMPRESSED: {
        chunk.data = &chunk_data[5]; // funny part is - this is probably slower than using deflate compression.
        chunk.data_size = compressed_size;
        return chunk;
    }
    case REGION_COMPRESSION_LZ4: {
        // minecraft's "LZ4" format is a custom format that does not conform to even the lz4 frame format.
        // AFAICT it's unlikely to ever be supported here, unless someone makes a C library for this stupid and uneccecary mutation.
    };
    case REGION_COMPRESSION_CUSTOM: {
        return chunk;
    }
    default: return chunk;
    }
}

int anvil_section_iter_init(
    struct anvil_section_iter *iter,
    struct anvil_chunk chunk
) {
    if (chunk.data_size < 3) {
        return -1;
    }

    char *sections = nbt_payload(nbt_named(
        nbt_payload(chunk.data, NBT_COMPOUND, chunk.data + chunk.data_size), 
        "sections", 
        chunk.data + chunk.data_size
    ), NBT_LIST, chunk.data + chunk.data_size);

    iter->next_section = nbt_list_payload(sections);
    iter->end = chunk.data + chunk.data_size;
    iter->section_count = nbt_list_size(sections);
    iter->current_index = -1; // uuh

    return anvil_section_iter_next(iter);
}

int anvil_section_iter_next(struct anvil_section_iter *iter) {
    iter->section_y = ~0;
    iter->block_state_palette = NULL;
    iter->block_state_array = NULL;
    iter->biome_palette = NULL;
    iter->biome_array = NULL;
    iter->block_light = NULL;
    iter->sky_light = NULL;

    if (++iter->current_index >= iter->section_count) {
        return 1;
    }
    
    if (iter->next_section == NULL) {
        return -1;
    }

    char *block_states = nbt_payload(nbt_named(iter->next_section, "block_states", iter->end), NBT_COMPOUND, iter->end);
    iter->block_state_palette = nbt_payload(nbt_named(block_states, "palette", iter->end), NBT_LIST, iter->end);
    iter->block_state_array = nbt_payload(nbt_named(block_states, "data", iter->end), NBT_LONG_ARRAY, iter->end);

    char *biomes = nbt_payload(nbt_named(iter->next_section, "biomes", iter->end), NBT_COMPOUND, iter->end);
    iter->biome_palette = nbt_payload(nbt_named(biomes, "palette", iter->end), NBT_LIST, iter->end);
    iter->biome_array = nbt_payload(nbt_named(biomes, "data", iter->end), NBT_LONG_ARRAY, iter->end);

    iter->block_light = nbt_payload(nbt_named(iter->next_section, "BlockLight", iter->end), NBT_BYTE_ARRAY, iter->end);
    iter->sky_light = nbt_payload(nbt_named(iter->next_section, "SkyLight", iter->end), NBT_BYTE_ARRAY, iter->end);

    iter->next_section = nbt_step(iter->next_section, iter->end);
    return 0;
}
