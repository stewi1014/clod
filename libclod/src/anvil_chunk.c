#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbit.h>

#include <libdeflate.h>

#include "nbt.h"
#include "anvil.h"
#include "anvil_world.h"

#define CHUNK_BUFFER_GROW(cap, n) 

#define BUFFER_START 100 * 1024
#define BUFFER_MAX 4 * 1024 * 1024



//=======//
// Chunk //
//=======//

struct anvil_chunk_ctx {
    char *buffer;
    size_t buffer_cap;

    struct libdeflate_decompressor *libdeflate_decompressor;

    void *(*realloc)(void*, size_t);
};

thread_local void *(*realloc_wrapped)(void*, size_t);

void *malloc_wrapper(size_t size) {
    return realloc_wrapped(NULL, size);
};

void free_wrapper(void *ptr) {
    realloc_wrapped(ptr, 0);
}

struct anvil_chunk_ctx *anvil_chunk_ctx_alloc(struct anvil_world *world) {
    struct anvil_chunk_ctx *ctx;
    if (world == NULL) {
        ctx = malloc(sizeof(struct anvil_chunk_ctx));
        if (ctx == NULL) return NULL;
        ctx->realloc = realloc;
    } else {
        ctx = world->realloc(NULL, sizeof(struct anvil_chunk_ctx));
        if (ctx == NULL) return NULL;
        ctx->realloc = world->realloc;
    }

    ctx->libdeflate_decompressor = NULL;
    ctx->buffer_cap = BUFFER_START;
    ctx->buffer = ctx->realloc(NULL, ctx->buffer_cap);
    if (ctx->buffer == NULL) {
        ctx->realloc(ctx, 0);
        return NULL;
    }

    return ctx;
}

void anvil_chunk_ctx_free(struct anvil_chunk_ctx *ctx) {
    if (ctx->libdeflate_decompressor != NULL) {
        realloc_wrapped = ctx->realloc;
        libdeflate_free_decompressor(ctx->libdeflate_decompressor);
        realloc_wrapped = NULL;
    }

    if (ctx->buffer != NULL) {
        ctx->realloc(ctx->buffer, 0);
    }

    ctx->realloc(ctx, 0);
}

static struct libdeflate_decompressor *chunk_ctx_libdeflate_decompressor(struct anvil_chunk_ctx *ctx) {
    if (ctx->libdeflate_decompressor == NULL) {
        struct libdeflate_options options;
        memset(&options, 0, sizeof(options));
        options.sizeof_options = sizeof(options);

        options.malloc_func = malloc_wrapper;
        options.free_func = free_wrapper;

        realloc_wrapped = ctx->realloc;
        ctx->libdeflate_decompressor = libdeflate_alloc_decompressor_ex(&options);
        realloc_wrapped = NULL;
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
    int64_t chunk_x,
    int64_t chunk_z
) {
    chunk_x &= 31;
    chunk_z &= 31;

    struct anvil_chunk chunk;
    chunk.data = NULL;
    chunk.data_size = 0;
    chunk.chunk_x = region->region_x * 32 + chunk_x;
    chunk.chunk_z = region->region_z * 32 + chunk_z;
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
            realloc_wrapped = ctx->realloc;
            result = libdeflate_gzip_decompress(
                decompressor, 
                &chunk_data[5], 
                compressed_size,
                ctx->buffer,
                ctx->buffer_cap,
                &decompressed_size
            );
            realloc_wrapped = NULL;
        } else if (chunk_data[4] == REGION_COMPRESSION_ZLIB) {
            realloc_wrapped = ctx->realloc;
            result = libdeflate_zlib_decompress(
                decompressor, 
                &chunk_data[5], 
                compressed_size,
                ctx->buffer,
                ctx->buffer_cap,
                &decompressed_size
            );
            realloc_wrapped = NULL;
        } else {
            assert(0);
        }

        if (result == LIBDEFLATE_INSUFFICIENT_SPACE){
            size_t new_capacity = ctx->buffer_cap * 2;
            char *new_buffer = ctx->realloc(ctx->buffer, new_capacity);
    
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

static
int read_block_state_array(
    uint16_t *block_state_indicies,
    char *block_state_array,
    int32_t block_states
) {
    uint16_t bits = stdc_bit_width_ui(block_states - 1) < 4 ? 4 : stdc_bit_width_ui(block_states - 1);
    uint16_t mask = ((~0ULL) >> (64 - bits));

    char *cursor = block_state_array + 4;

    for (unsigned i = 0; i < 4096;) {
        uint64_t n =
            ((uint64_t)(unsigned char)cursor[0] << 56) +
            ((uint64_t)(unsigned char)cursor[1] << 48) +
            ((uint64_t)(unsigned char)cursor[2] << 40) +
            ((uint64_t)(unsigned char)cursor[3] << 32) +
            ((uint64_t)(unsigned char)cursor[4] << 24) +
            ((uint64_t)(unsigned char)cursor[5] << 16) +
            ((uint64_t)(unsigned char)cursor[6] << 8) +
            ((uint64_t)(unsigned char)cursor[7]);

        cursor += 8;

        for (unsigned j = 0; j <= 64 - bits && i < 4096; j += bits, i++) {
            block_state_indicies[i] = (n >> j) & mask;
            if (block_state_indicies[i] > block_states) {
                #ifndef NDEBUG
                return -1;
                #else
                block_state_indicies[i] = block_states - 1;
                #endif
            }
        }
    }

    return 0;
}

static
int read_biome_array(
    uint16_t *biome_indicies,
    char *biome_array,
    int32_t biomes
) {
    uint16_t bits = stdc_bit_width_ui(biomes - 1);
    uint16_t mask = ((~0ULL) >> (64 - bits));

    char *cursor = biome_array + 4;

    for (int i = 0; i < 64;) {
        uint64_t n =
            ((uint64_t)(unsigned char)cursor[0] << 56) +
            ((uint64_t)(unsigned char)cursor[1] << 48) +
            ((uint64_t)(unsigned char)cursor[2] << 40) +
            ((uint64_t)(unsigned char)cursor[3] << 32) +
            ((uint64_t)(unsigned char)cursor[4] << 24) +
            ((uint64_t)(unsigned char)cursor[5] << 16) +
            ((uint64_t)(unsigned char)cursor[6] << 8) +
            ((uint64_t)(unsigned char)cursor[7]);

        cursor += 8;

        for (unsigned j = 0; j <= 64 - bits && i < 64; j += bits, i++) {
            biome_indicies[i] = (n >> j) & mask;
            if (biome_indicies[i] > biomes) {
                #ifndef NDEBUG
                return -1;
                #else
                biome_indicies[i] = biomes - 1;
                #endif
            }
        }
    }

    return 0;
}

int anvil_parse_sections_ex(
    struct anvil_sections *sections,
    struct anvil_chunk chunk,
    void *(*realloc_f)(void*, size_t)
) {
    if (chunk.data_size == 0) {
        sections->len = 0;
        return 0;
    }

    if (chunk.data_size < 3) {
        sections->len = 0;
        return -1;
    }

    sections->realloc = realloc_f != NULL ? realloc_f : realloc;

    char *section_list = NULL, *end;
    int64_t section_min_y = INT64_MIN;
    end = nbt_named(nbt_payload(chunk.data, NBT_COMPOUND, chunk.data + chunk.data_size), chunk.data + chunk.data_size,
        "sections", strlen("sections"), NBT_LIST, &section_list,
        "xPos", strlen("xPos"), NBT_ANY_INTEGER, &sections->x,
        "yPos", strlen("yPos"), NBT_ANY_INTEGER, &section_min_y,
        "zPos", strlen("zPos"), NBT_ANY_INTEGER, &sections->z,
        "Status", strlen("Status"), NBT_STRING, &sections->status,
        NULL
    );

    if (section_list == NULL || end == NULL || section_min_y == INT64_MIN)
        return -1;

    int32_t section_count = nbt_list_size(section_list);

    if (sections->section == NULL || sections->cap < section_count) {
        struct anvil_section *new = sections->realloc(sections->section, section_count * sizeof(struct anvil_section));
        if (new == NULL) {
            return -1;
        }

        for (int64_t i = sections->cap; i < section_count; i++) {
            new[i].end = NULL;
            new[i].block_state_palette = NULL;
            new[i].block_state_indicies = NULL;
            new[i].biome_palette = NULL;
            new[i].biome_indicies = NULL;
            new[i].block_light = NULL;
            new[i].sky_light = NULL;
        }

        sections->section = new;
        sections->cap = section_count;
    }

    for (int64_t i = 0; i < section_count; i++) {
        sections->section[i].end = NULL;
        sections->section[i].block_state_palette = NULL;
        sections->section[i].biome_palette = NULL;
        sections->section[i].block_light = NULL;
        sections->section[i].sky_light = NULL;
    }

    sections->len = section_count;
    sections->min_y = section_min_y;
    sections->start = nbt_list_payload(section_list);

    char *section_tag = nbt_list_payload(section_list);
    for (int index = 0; index < section_count; index++) {
        char *biomes = NULL;
        char *block_states = NULL;
        char *block_light = NULL;
        char *sky_light = NULL;
        char *biome_array = NULL;
        char *block_state_array = NULL;
        
        int64_t y = INT64_MIN;

        section_tag = nbt_named(section_tag, end,
            "biomes", strlen("biomes"), NBT_COMPOUND, &biomes,
            "block_states", strlen("block_states"), NBT_COMPOUND, &block_states,
            "BlockLight", strlen("BlockLight"), NBT_BYTE_ARRAY, &block_light,
            "SkyLight", strlen("SkyLight"), NBT_BYTE_ARRAY, &sky_light,
            "Y", strlen("Y"), NBT_ANY_INTEGER, &y,
            NULL
        );
        if (section_tag == NULL) return -1;
        if (y < section_min_y || y >= (section_min_y + section_count)) continue;

        struct anvil_section *section = &sections->section[y - section_min_y];

        sections->section[y - section_min_y].end = section_tag;

        if (block_light != NULL && nbt_byte_array_size(block_light) == 2048)
            sections->section[y - section_min_y].block_light = nbt_byte_array(block_light);
        else
            sections->section[y - section_min_y].block_light = NULL;

        if (sky_light != NULL && nbt_byte_array_size(sky_light) == 2048)
            sections->section[y - section_min_y].sky_light = nbt_byte_array(sky_light);
        else
            sections->section[y - section_min_y].sky_light = NULL;

        nbt_named(biomes, end,
            "palette", strlen("palette"), NBT_LIST, &section->biome_palette,
            "data", strlen("data"), NBT_LONG_ARRAY, &biome_array,
            NULL
        );
    
        nbt_named(block_states, end,
            "palette", strlen("palette"), NBT_LIST, &section->block_state_palette,
            "data", strlen("data"), NBT_LONG_ARRAY, &block_state_array,
            NULL
        );

        int biome_count = section->biome_palette == NULL ? 0 : nbt_list_size(section->biome_palette);
        if (biome_count > 1) {
            if (biome_array == NULL) {
                return -1;
            }

            if (section->biome_indicies == NULL) {
                section->biome_indicies = sections->realloc(NULL, 4 * 4 * 4 * sizeof(uint16_t));
                if (section->biome_indicies == NULL) {
                    return -1;
                }
            }

            int error = read_biome_array(
                section->biome_indicies, 
                biome_array,
                biome_count
            );
            if (error) {
                return -1;
            }
        }

        int block_state_count = section->block_state_palette == NULL ? 0 : nbt_list_size(section->block_state_palette);
        if (block_state_count > 1) {
            if (block_state_array == NULL) {
                return -1;
            }

            if (section->block_state_indicies == NULL) {
                section->block_state_indicies = sections->realloc(NULL, 16 * 16 * 16 * sizeof(uint16_t));
                if (section->block_state_indicies == NULL) {
                    return -1;
                }
            }

            int error = read_block_state_array(
                section->block_state_indicies,
                block_state_array, 
                block_state_count
            );

            if (error) {
                return -1;
            }
        }
    }

    return 0;
}


void anvil_sections_free(
    struct anvil_sections *sections
) {
    for (int64_t i = 0; i < sections->cap; i++) {
        if (sections->section[i].biome_indicies != NULL)
            sections->realloc(sections->section[i].biome_indicies, 0);

        if (sections->section[i].block_state_indicies != NULL)
            sections->realloc(sections->section[i].block_state_indicies, 0);
    }

    sections->realloc(sections->section, 0);
    sections->cap = 0;
    sections->len = 0;
    sections->section = NULL;
}
