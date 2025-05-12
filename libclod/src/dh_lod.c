#include <stdbool.h>
#include <stdlib.h>

#include <anvil.h>
#include <nbt.h>

#include "compress.h"
#include "dh.h"

#define LOD_GROW(cap, n) ((n) + (cap << 1) - (cap >> 1))
#define LOD_SHRINK(len, cap) ((cap))

#define DH_DATAPOINT_BLOCK_LIGHT_MASK   (0xF000000000000000ULL)
#define DH_DATAPOINT_SKY_LIGHT_MASK     (0x0F00000000000000ULL)
#define DH_DATAPOINT_MIN_Y_MASK         (0x00FFF00000000000ULL)
#define DH_DATAPOINT_HEIGHT_MASK        (0x00000FFF00000000ULL)
#define DH_DATAPOINT_ID_MASK            (0x00000000FFFFFFFFULL)

#define DH_DATAPOINT_BLOCK_LIGHT_SHIFT  (60)
#define DH_DATAPOINT_SKY_LIGHT_SHIFT    (56)
#define DH_DATAPOINT_MIN_Y_SHIFT        (44)
#define DH_DATAPOINT_HEIGHT_SHIFT       (32)
#define DH_DATAPOINT_ID_SHIFT           (00)

#define DH_DATAPOINT_GET_BLOCK_LIGHT(dp) ((dp & DH_DATAPOINT_BLOCK_LIGHT_MASK ) >> DH_DATAPOINT_BLOCK_LIGHT_SHIFT)
#define DH_DATAPOINT_GET_SKY_LIGHT(dp)   ((dp & DH_DATAPOINT_SKY_LIGHT_MASK   ) >> DH_DATAPOINT_SKY_LIGHT_SHIFT  )
#define DH_DATAPOINT_GET_MIN_Y(dp)       ((dp & DH_DATAPOINT_MIN_Y_MASK       ) >> DH_DATAPOINT_MIN_Y_SHIFT      )
#define DH_DATAPOINT_GET_HEIGHT(dp)      ((dp & DH_DATAPOINT_HEIGHT_MASK      ) >> DH_DATAPOINT_HEIGHT_SHIFT     )
#define DH_DATAPOINT_GET_ID(dp)          ((dp & DH_DATAPOINT_ID_MASK          ) >> DH_DATAPOINT_ID_SHIFT         )

#define DH_DATAPOINT_SET_BLOCK_LIGHT(dp, v) ((dp &~ DH_DATAPOINT_BLOCK_LIGHT_MASK) | ((((uint64_t)(v)) << DH_DATAPOINT_BLOCK_LIGHT_SHIFT ) & DH_DATAPOINT_BLOCK_LIGHT_MASK))
#define DH_DATAPOINT_SET_SKY_LIGHT(dp, v)   ((dp &~ DH_DATAPOINT_SKY_LIGHT_MASK  ) | ((((uint64_t)(v)) << DH_DATAPOINT_SKY_LIGHT_SHIFT   ) & DH_DATAPOINT_SKY_LIGHT_MASK  ))
#define DH_DATAPOINT_SET_MIN_Y(dp, v)       ((dp &~ DH_DATAPOINT_MIN_Y_MASK      ) | ((((uint64_t)(v)) << DH_DATAPOINT_MIN_Y_SHIFT       ) & DH_DATAPOINT_MIN_Y_MASK      ))
#define DH_DATAPOINT_SET_HEIGHT(dp, v)      ((dp &~ DH_DATAPOINT_HEIGHT_MASK     ) | ((((uint64_t)(v)) << DH_DATAPOINT_HEIGHT_SHIFT      ) & DH_DATAPOINT_HEIGHT_MASK     ))
#define DH_DATAPOINT_SET_ID(dp, v)          ((dp &~ DH_DATAPOINT_ID_MASK         ) | ((((uint64_t)(v)) << DH_DATAPOINT_ID_SHIFT          ) & DH_DATAPOINT_ID_MASK         ))

struct id_lookup {
    struct id_table {
        uint32_t *ids;
        size_t ids_cap;
        uint16_t air_block_state;
    } *sections;
    size_t sections_cap;
};

#define ID_LOOKUP_CLEAR (struct id_lookup){NULL, 0}

struct dh_lod_ext {
    char *temp_string;
    size_t temp_string_cap;

    char **temp_array;
    size_t temp_array_cap;

    char *temp_buffer;
    size_t temp_buffer_cap;

    char *big_buffer;
    size_t big_buffer_cap;

    void *lzma_ctx;
    void *lz4_ctx;

    struct anvil_sections sections[4];
    struct id_lookup id_lookup[4];
};

int compare_tag_name(const void *tag1, const void *tag2) {
    size_t size = nbt_name_size((char*)tag1, NULL);
    if (nbt_name_size((char*)tag2, NULL) < size) {
        size = nbt_name_size((char*)tag2, NULL);
    }

    int n = strncmp(nbt_name((char*)tag1, NULL), nbt_name((char*)tag2, NULL), size);
    if (n) return n;
    if (nbt_name_size((char*)tag1, NULL) > nbt_name_size((char*)tag2, NULL)) return 1;
    return -1;
}

static inline
void increment_be_uint16_t(char *ptr) {
    uint16_t value = 
        (((uint8_t)ptr[0] << (1 * 8)) & 0xFF) |
        (((uint8_t)ptr[1] << (0 * 8)) & 0xFF);

    value++;

    ptr[0] = (char)((value >> (1 * 8)) & 0xFF);
    ptr[1] = (char)((value >> (0 * 8)) & 0xFF);
}

/**
 * general idea here is to accumulate permutations of biome and blockstate/properties,
 * creating a DH compatable mapping and then updating a minecarft id -> DH id lookup table.
 * 
 * this does not need to be super optimised, and given the complexity of this transformation,
 * readabily is more important here.
 * 
 * I would like to see the need for this removed from LODs entirely.
 * 
 */
dh_result add_mappings(
    struct dh_lod *lod,
    struct anvil_sections *sections,
    struct id_lookup *id_table
) {

    // C might be a *right* proper pain with strings,
    // but it does give us macros.
    //
    // this macro just helps us easily append to the temporary string,
    // while checking size, growing if needed and keeping track of offset.
    // I know it's weird, but a function doing the same thing looked worse IMHO.
    //
    // A decent string abstraction would be better.
    #define append(off, src, src_len) \
        if (ext->temp_string_cap < (off) + (src_len)) {\
            char *new = lod->realloc(ext->temp_string, (off) + (src_len));\
            if (new == NULL) return DH_ERR_ALLOC;\
            ext->temp_string = new;\
            ext->temp_string_cap = (off) + (src_len);\
        }\
        memcpy(ext->temp_string + (off), (src), (src_len));\
        (off) += (src_len);

    
    struct dh_lod_ext *ext = lod->__ext;

    if (id_table->sections_cap < sections->len) {
        size_t new_cap = sections->len;
        struct id_table *new = lod->realloc(id_table->sections, new_cap * sizeof(*id_table->sections));
        if (new == NULL) {
            return DH_ERR_ALLOC;
        }

        for (int64_t i = id_table->sections_cap; i < new_cap; i++) {
            new[i].ids_cap = 0;
            new[i].ids = NULL;
        }

        id_table->sections = new;
        id_table->sections_cap = new_cap;
    }

    for (int64_t section_index = 0; section_index < sections->len; section_index++) {
        struct anvil_section section = sections->section[section_index];

        if (section.biome_palette == NULL || section.block_state_palette == NULL)
            continue;

        int32_t biomes = nbt_list_size(section.biome_palette);
        if (biomes > 64 || biomes < 0) {
            return DH_ERR_MALFORMED;
        }

        int32_t block_states = nbt_list_size(section.block_state_palette);
        if (block_states > 4096 || block_states < 0) {
            return DH_ERR_MALFORMED;
        }

        if (id_table->sections[section_index].ids_cap < biomes * block_states) {
            size_t new_cap = biomes * block_states;
            uint32_t *new = lod->realloc(
                id_table->sections[section_index].ids, 
                new_cap * sizeof(*id_table->sections[section_index].ids)
            );

            if (new == NULL) {
                return DH_ERR_ALLOC;
            }

            id_table->sections[section_index].ids = new;
            id_table->sections[section_index].ids_cap = new_cap;
        }

        char *biome = nbt_list_payload(section.biome_palette);
        for (
            int64_t biome_index = 0;
            biome_index < biomes;
            biome = nbt_payload_step(biome, NBT_STRING, section.end), biome_index++
        ) {
            size_t biome_string_size = 0;
            append(biome_string_size, nbt_string(biome), nbt_string_size(biome));
            append(biome_string_size, "_DH-BSW_", strlen("_DH-BSW_"));

            char *block_state = nbt_list_payload(section.block_state_palette);
            for (
                int64_t block_state_index = 0;
                block_state_index < block_states;
                block_state = nbt_payload_step(block_state, NBT_COMPOUND, section.end), block_state_index++
            ) {
                char *block_state_name = NULL, *block_state_property = NULL;
                nbt_named(block_state, section.end,
                    "Name", strlen("Name"), NBT_STRING, &block_state_name,
                    "Properties", strlen("Properties"), NBT_COMPOUND, &block_state_property,
                    NULL
                );

                size_t block_state_string_size = biome_string_size;
                append(block_state_string_size, nbt_string(block_state_name), nbt_string_size(block_state_name));
                append(block_state_string_size, "_STATE_", strlen("_STATE_"));

                if (
                    strlen("minecraft:air") == nbt_string_size(block_state_name) &&
                    !strncmp(nbt_string(block_state_name), "minecraft:air", nbt_string_size(block_state_name))
                ) {
                    id_table->sections[section_index].air_block_state = block_state_index;
                }

                if (block_state_property != NULL) {
                    int64_t property_count = 0;
                    while (block_state_property[0] == NBT_STRING) {
                        if (ext->temp_array_cap == property_count) {
                            char **new = lod->realloc(ext->temp_array, (2 * ext->temp_array_cap + 2) * sizeof(char*));
                            if (new == NULL) {
                                return DH_ERR_ALLOC;
                            }
                            ext->temp_array = new;
                            ext->temp_array_cap = 2 * ext->temp_array_cap + 2;
                        }

                        ext->temp_array[property_count] = block_state_property;
                        block_state_property = nbt_step(block_state_property, section.end);
                    }

                    // every language has a sort method
                    // this is C's

                    qsort(ext->temp_array, property_count, sizeof(char*), compare_tag_name);

                    for (
                        int property_index = 0; 
                        property_index < property_count; 
                        block_state_property = nbt_step(block_state_property, section.end), property_index++
                    ) {
                        append(block_state_string_size, "{", 1);
                        append(block_state_string_size, nbt_name(block_state_property, section.end), nbt_name_size  (block_state_property, section.end));
                        append(block_state_string_size, ":", 1);
                        append(
                            block_state_string_size, 
                            nbt_string(nbt_payload(block_state_property, NBT_STRING, section.end)),
                            nbt_string_size(nbt_payload(block_state_property, NBT_STRING, section.end))
                        );
                        append(block_state_string_size, "}", 1);
                    }
                }

                append(block_state_string_size, "\x0", 1);

                uint32_t id = 0;
                while (id < lod->mapping_len && strcmp(lod->mapping_arr[id], ext->temp_string))
                    id++;

                if (id == lod->mapping_len) {
                    if (lod->mapping_len == lod->mapping_cap) {
                        size_t new_cap = lod->mapping_cap == 0 ? 16 : lod->mapping_cap * 2;
                        char **new = lod->realloc(lod->mapping_arr, new_cap * sizeof(char*));
                        if (new == NULL) {
                            return DH_ERR_ALLOC;
                        }
                        for (int64_t i = lod->mapping_cap; i < new_cap; i++) new[i] = NULL;
                        lod->mapping_arr = new;
                        lod->mapping_cap = new_cap;
                    }
                    
                    char *new = lod->realloc(lod->mapping_arr[id], block_state_string_size);
                    if (new == NULL) {
                        return DH_ERR_ALLOC;
                    }

                    strcpy(new, ext->temp_string);
                    lod->mapping_arr[id] = new;
                    lod->mapping_len++;
                }

                id_table->sections[section_index].ids[biome_index * nbt_list_size(section.block_state_palette) + block_state_index] = id;
            }
        }
    }

    #undef append

    return DH_OK;
}

dh_result dh_from_chunks(
    struct anvil_chunk *chunks,  // 4x4 array of chunks.
    struct dh_lod *lod          // destination LOD.
) {
    struct dh_lod_ext *ext;
    dh_result result;
    char *cursor;
    int error;

    if (chunks == NULL || lod == NULL) return DH_ERR_INVALID_ARGUMENT;
    if (lod->realloc == NULL) lod->realloc = realloc;

    #define ensure_buffer(n) ({\
        if (lod->lod_cap < lod->lod_len + (n)) {\
            size_t new_cap = LOD_GROW(lod->lod_cap, n);\
            char *new = lod->realloc(lod->lod_arr, new_cap);\
            if (new == NULL) {\
                return DH_ERR_ALLOC;\
            }\
            lod->lod_arr = new;\
            lod->lod_cap = new_cap;\
        }\
    })

    ext = lod->__ext;
    if (ext == NULL) {
        ext = lod->realloc(NULL, sizeof(struct dh_lod_ext));
        if (ext == NULL) {
            return DH_ERR_ALLOC;
        }

        ext->temp_string = NULL;
        ext->temp_string_cap = 0;

        ext->temp_array = NULL;
        ext->temp_array_cap = 0;

        ext->temp_buffer = NULL;
        ext->temp_buffer_cap = 0;

        ext->big_buffer = NULL;
        ext->big_buffer_cap = 0;

        ext->lzma_ctx = NULL;
        ext->lz4_ctx = NULL;

        for (int64_t i = 0; i < 4; i++) {
            ext->sections[i] = ANVIL_SECTIONS_CLEAR;
            ext->id_lookup[i] = ID_LOOKUP_CLEAR;
        }

        lod->__ext = ext;
    }

    if (ext->big_buffer_cap > lod->lod_cap) {
        char *tmp = lod->lod_arr;
        lod->lod_arr = ext->big_buffer;
        ext->big_buffer = tmp;

        size_t tmp_cap = lod->lod_cap;
        lod->lod_cap = ext->big_buffer_cap;
        ext->big_buffer_cap = tmp_cap;
    }

    lod->mapping_len = 0;
    lod->lod_len = 0;
    lod->detail_level = 0;
    lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;
    lod->x = chunks->chunk_x / 4;
    lod->z = chunks->chunk_z / 4;
    lod->has_data = false;

    for (int64_t chunk_x = 0; chunk_x < 4; chunk_x++) {

        for (int64_t chunk_z = 0; chunk_z < 4; chunk_z++) {
            error = anvil_parse_sections_ex(
                &ext->sections[chunk_z],
                chunks[chunk_x * 4 + chunk_z],
                lod->realloc
            );

            if (error) {
                return DH_ERR_MALFORMED;
            }

            result = add_mappings(
                lod,
                &ext->sections[chunk_z],
                &ext->id_lookup[chunk_z]
            );

            if (result != DH_OK) {
                return result;
            }
        }

        if (chunk_x == 0) {
            lod->min_y = ext->sections->min_y * 16;
        } else if (lod->min_y != ext->sections->min_y * 16) {
            return DH_ERR_MALFORMED;
        }

        for (int64_t block_x = 0; block_x < 16; block_x++)
        for (int64_t chunk_z = 0; chunk_z < 4; chunk_z++) {

            struct anvil_sections *sections = &ext->sections[chunk_z];
            struct id_lookup *id_lookup = &ext->id_lookup[chunk_z];

            for (int64_t block_z = 0; block_z < 16; block_z++) {

                ensure_buffer(2 + 8 * sections->len * 16);

                size_t column_len_off = lod->lod_len;
                cursor = lod->lod_arr + lod->lod_len;
                cursor[0] = 0;
                cursor[1] = 0;
                lod->lod_len += 2;
            
                if (
                    sections->status != NULL && (
                    strlen("minecraft:full") != nbt_string_size(sections->status) ||
                    0 != strncmp("minecraft:full", nbt_string(sections->status), strlen("minecraft:full"))
                )) {
                    continue;
                }
            
                uint64_t last_datapoint = 
                    0xFULL                  << DH_DATAPOINT_SKY_LIGHT_SHIFT |
                    (sections->len * 16)    << DH_DATAPOINT_MIN_Y_SHIFT     ;
        
                uint64_t this_datapoint = 0;

                uint64_t next_datapoint = 
                    1ULL << DH_DATAPOINT_HEIGHT_SHIFT;
            
                for (int64_t section_index = sections->len - 1; section_index >= 0; section_index--){
                    struct anvil_section *section = &sections->section[section_index];
            
                    if (section->biome_palette == NULL || section->block_state_palette == NULL)
                        continue;
            
                    uint32_t *id_table = id_lookup->sections[section_index].ids;
            
                    int32_t biome_count = nbt_list_size(section->biome_palette);
                    int32_t block_state_count = nbt_list_size(section->block_state_palette);
            
                    for (int64_t block_y = 15; block_y >= 0; block_y--){
                        int64_t index = block_y * 16 * 16 + block_z * 16 + block_x;
            
                        int32_t biome = 0;
                        if (biome_count > 1) 
                            biome = section->biome_indicies[(block_y / 4) * 4 * 4 + (block_z / 4) * 4 + (block_x / 4)];
            
                        int32_t block_state = 0;
                        if (block_state_count > 1)
                            block_state = section->block_state_indicies[index];
            
                        assert(biome < biome_count);
                        assert(block_state < block_state_count);
            
                        this_datapoint = next_datapoint;

                        if (section->sky_light != NULL) {
                            next_datapoint = DH_DATAPOINT_SET_SKY_LIGHT(
                                next_datapoint, 
                                (section->sky_light[index / 2] >> ((index & 1) * 4)) & 0xF
                            );
                        }

                        if (section->block_light != NULL) {
                            next_datapoint = DH_DATAPOINT_SET_BLOCK_LIGHT(
                                next_datapoint, 
                                (section->block_light[index / 2] >> ((index & 1) * 4)) & 0xF
                            );
                        } else {
                            next_datapoint = DH_DATAPOINT_SET_BLOCK_LIGHT(next_datapoint, 0);
                        }
                        
                        uint32_t id = id_table[biome * block_state_count + block_state];
                        if ((last_datapoint & DH_DATAPOINT_ID_MASK) >> DH_DATAPOINT_ID_SHIFT == id) {
                            last_datapoint += 
                                ((uint64_t)1 << DH_DATAPOINT_HEIGHT_SHIFT) + 
                                ((uint64_t)-1 << DH_DATAPOINT_MIN_Y_SHIFT);
            
                            continue;
                        }
            
                        if ((last_datapoint & DH_DATAPOINT_HEIGHT_MASK) >> DH_DATAPOINT_HEIGHT_SHIFT > 0) {
                            lod->has_data = true;
                            cursor = lod->lod_arr + lod->lod_len;
                            cursor[0] = (last_datapoint >> (7 * 8)) & 0xFF;
                            cursor[1] = (last_datapoint >> (6 * 8)) & 0xFF;
                            cursor[2] = (last_datapoint >> (5 * 8)) & 0xFF;
                            cursor[3] = (last_datapoint >> (4 * 8)) & 0xFF;
                            cursor[4] = (last_datapoint >> (3 * 8)) & 0xFF;
                            cursor[5] = (last_datapoint >> (2 * 8)) & 0xFF;
                            cursor[6] = (last_datapoint >> (1 * 8)) & 0xFF;
                            cursor[7] = (last_datapoint >> (0 * 8)) & 0xFF;
                            lod->lod_len += 8;
                            increment_be_uint16_t(lod->lod_arr + column_len_off);
                        }
            
                        last_datapoint =
                            this_datapoint                                                              |
                            (uint64_t)(section_index * 16 + block_y)  << DH_DATAPOINT_MIN_Y_SHIFT       |
                            (uint64_t)id                              << DH_DATAPOINT_ID_SHIFT          ;
                    }
                }
            
                if ((last_datapoint & DH_DATAPOINT_HEIGHT_MASK) >> DH_DATAPOINT_HEIGHT_SHIFT > 0) {
                    lod->has_data = true;
                    cursor = lod->lod_arr + lod->lod_len;
                    cursor[0] = (last_datapoint >> (7 * 8)) & 0xFF;
                    cursor[1] = (last_datapoint >> (6 * 8)) & 0xFF;
                    cursor[2] = (last_datapoint >> (5 * 8)) & 0xFF;
                    cursor[3] = (last_datapoint >> (4 * 8)) & 0xFF;
                    cursor[4] = (last_datapoint >> (3 * 8)) & 0xFF;
                    cursor[5] = (last_datapoint >> (2 * 8)) & 0xFF;
                    cursor[6] = (last_datapoint >> (1 * 8)) & 0xFF;
                    cursor[7] = (last_datapoint >> (0 * 8)) & 0xFF;
                    lod->lod_len += 8;
                    increment_be_uint16_t(lod->lod_arr + column_len_off);
                }

            }
        }
    }

    size_t shrink_cap = LOD_SHRINK(lod->lod_len, lod->lod_cap);
    if (shrink_cap < lod->lod_cap) {
        char *new = lod->realloc(lod->lod_arr, shrink_cap);
        // don't really care if shrinking fails.
        if (new != NULL) {
            lod->lod_arr = new;
            lod->lod_cap = shrink_cap;
        }
    }

    #undef ensure_buffer

    return DH_OK;
}

dh_result dh_from_lods(
    struct dh_lod *lods, // 2x2 array of source LODs.
    struct dh_lod *lod  // destination LOD.
) {
    if (lods == NULL || lod == NULL) return DH_ERR_INVALID_ARGUMENT;
    for (int64_t i = 0; i < 4; i++) if (lods[i].realloc == NULL) lods[i].realloc = realloc;
    if (lod->realloc == NULL) lod->realloc = realloc;

    return DH_ERR_ALLOC;
}

dh_result dh_lod_serialise_mapping(
    struct dh_lod *lod,
    char **out,
    size_t *nbytes
) {
    struct dh_lod_ext *ext = lod->__ext;
    *nbytes = 0;

    #define ensure_buffer(n) \
        if (ext->temp_buffer_cap < (n) + (*nbytes)) {\
            size_t new_cap = (n) + ext->temp_buffer_cap * 2;\
            char *new = lod->realloc(ext->temp_buffer, new_cap);\
            if (new == NULL) {\
                return DH_ERR_ALLOC;\
            }\
            ext->temp_buffer_cap = new_cap;\
            ext->temp_buffer = new;\
        }\

    ensure_buffer(4);
    ext->temp_buffer[(*nbytes)++] = (lod->mapping_len >> (3 * 8)) & 0xFF;
    ext->temp_buffer[(*nbytes)++] = (lod->mapping_len >> (2 * 8)) & 0xFF;
    ext->temp_buffer[(*nbytes)++] = (lod->mapping_len >> (1 * 8)) & 0xFF;
    ext->temp_buffer[(*nbytes)++] = (lod->mapping_len >> (0 * 8)) & 0xFF;
    
    for (int64_t i = 0; i < lod->mapping_len; i++) {
        size_t size = strlen(lod->mapping_arr[i]);
        if (size > UINT16_MAX) {
            return DH_ERR_MALFORMED;
        }

        ensure_buffer(2 + size);
        ext->temp_buffer[(*nbytes)++] = (size >> (1 * 8)) & 0xFF;
        ext->temp_buffer[(*nbytes)++] = (size >> (0 * 8)) & 0xFF;
        memcpy(&ext->temp_buffer[*nbytes], lod->mapping_arr[i], size);
        *nbytes += size;
    }

    #undef ensure_buffer

    switch (lod->compression_mode) {
    case DH_DATA_COMPRESSION_UNCOMPRESSED: {
        *out = ext->temp_buffer;
        return DH_OK;
    }
    case DH_DATA_COMPRESSION_LZ4: {
        int result;
        size_t compressed_mapping_len;

        result = compress_lz4(
            &ext->lz4_ctx,
            ext->temp_buffer,
            *nbytes,
            &ext->temp_string,
            &ext->temp_string_cap,
            &compressed_mapping_len,
            lod->realloc
        );

        if (result != 0) {
            *nbytes = 0;
            return DH_ERR_COMPRESS;
        }

        *out = ext->temp_string;
        *nbytes = compressed_mapping_len;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_LZMA2: {
        lzma_ret result;
        size_t compressed_mapping_len;

        result = compress_lzma(
            &ext->lzma_ctx,
            ext->temp_buffer,
            *nbytes,
            &ext->temp_string,
            &ext->temp_string_cap,
            &compressed_mapping_len,
            lod->realloc
        );

        if (result != LZMA_OK && result != LZMA_STREAM_END) {
            *nbytes = 0;
            return DH_ERR_COMPRESS;
        }

        *out = ext->temp_string;
        *nbytes = compressed_mapping_len;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_ZSTD: {
        return DH_ERR_UNSUPPORTED;
    }
    default: {
        return DH_ERR_INVALID_ARGUMENT;
    }
    }
}

dh_result dh_compress(
    struct dh_lod *lod,
    int64_t compression_mode,
    double level
) {
    struct dh_lod_ext *ext = lod->__ext;

    if (compression_mode == lod->compression_mode)
        return DH_OK;
    
    char  *decompressed_lod_arr;
    size_t decompressed_lod_len;
    size_t decompressed_lod_cap;

    // existing format -> decompressed
    // this step consumes the data in the LOD,
    // writing the uncompressed data into the decompressed buffer.
    // after this step, the LOD's data and mapping is assumed to be invalid.
    // 

    switch (lod->compression_mode) {
    case DH_DATA_COMPRESSION_UNCOMPRESSED: {
        decompressed_lod_arr = lod->lod_arr;
        decompressed_lod_len = lod->lod_len;
        decompressed_lod_cap = lod->lod_cap;
        break;
    }
    case DH_DATA_COMPRESSION_LZ4: {
        return DH_ERR_UNSUPPORTED;
    }
    case DH_DATA_COMPRESSION_LZMA2: {
        return DH_ERR_UNSUPPORTED;
    }
    case DH_DATA_COMPRESSION_ZSTD: {
        return DH_ERR_UNSUPPORTED;
    }
    default: {
        return DH_ERR_INVALID_ARGUMENT;
    }
    }


    switch (compression_mode) {
    case DH_DATA_COMPRESSION_UNCOMPRESSED: {
        lod->lod_arr = decompressed_lod_arr;
        lod->lod_len = decompressed_lod_len;
        lod->lod_cap = decompressed_lod_cap;

        lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_LZ4: {
        int result;
        size_t compressed_lod_len;

        result = compress_lz4(
            &ext->lz4_ctx,
            decompressed_lod_arr,
            decompressed_lod_len,
            &ext->big_buffer,
            &ext->big_buffer_cap,
            &compressed_lod_len,
            lod->realloc
        );

        if (result != 0) {
            lod->lod_arr = decompressed_lod_arr;
            lod->lod_len = decompressed_lod_len;
            lod->lod_cap = decompressed_lod_cap;
    
            lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;
    
            return DH_ERR_COMPRESS;
        }

        lod->lod_arr = ext->big_buffer;
        lod->lod_len = compressed_lod_len;
        lod->lod_cap = ext->big_buffer_cap;

        ext->big_buffer = decompressed_lod_arr;
        ext->big_buffer_cap = decompressed_lod_cap;

        lod->compression_mode = DH_DATA_COMPRESSION_LZ4;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_LZMA2: {
        lzma_ret result;
        size_t compressed_lod_len;

        result = compress_lzma(
            &ext->lzma_ctx,
            decompressed_lod_arr,
            decompressed_lod_len,
            &ext->big_buffer,
            &ext->big_buffer_cap,
            &compressed_lod_len,
            lod->realloc
        );

        if (result != LZMA_OK && result != LZMA_STREAM_END) {
            lod->lod_arr = decompressed_lod_arr;
            lod->lod_len = decompressed_lod_len;
            lod->lod_cap = decompressed_lod_cap;
    
            lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;
    
            return DH_ERR_COMPRESS;
        }

        lod->lod_arr = ext->big_buffer;
        lod->lod_len = compressed_lod_len;
        lod->lod_cap = ext->big_buffer_cap;

        ext->big_buffer = decompressed_lod_arr;
        ext->big_buffer_cap = decompressed_lod_cap;

        lod->compression_mode = DH_DATA_COMPRESSION_LZMA2;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_ZSTD: {
        return DH_ERR_UNSUPPORTED;
    }
    default: {
        return DH_ERR_INVALID_ARGUMENT;
    }
    }
}

void dh_lod_free(
    struct dh_lod *lod
) {
    if (lod == NULL) return;
    if (lod->realloc == NULL) return;

    for (int64_t i = 0; i < lod->mapping_cap; i++) lod->realloc(lod->mapping_arr[i], 0);
    lod->realloc(lod->mapping_arr, 0);

    lod->realloc(lod->lod_arr, 0);

    struct dh_lod_ext *ext = lod->__ext;
    if (ext != NULL) {
        if (ext->temp_string != NULL) lod->realloc(ext->temp_string, 0);
        if (ext->temp_array != NULL) lod->realloc(ext->temp_array, 0);
        if (ext->temp_buffer != NULL) lod->realloc(ext->temp_buffer, 0);
        if (ext->big_buffer != NULL) lod->realloc(ext->big_buffer, 0);

        for (int64_t i = 0; i < 4; i++)
            anvil_sections_free(&ext->sections[i]);
        
        for (int64_t i = 0; i < 4; i++) {
            if (ext->id_lookup[i].sections != NULL) {
                for (int64_t j = 0; j < ext->id_lookup->sections_cap; j++) {
                    lod->realloc(ext->id_lookup[i].sections[j].ids, 0);
                }
                lod->realloc(ext->id_lookup[i].sections, 0);
            }
        }

        if (ext->lzma_ctx != NULL) compress_free_lzma(&ext->lzma_ctx, lod->realloc);
        if (ext->lz4_ctx != NULL) compress_free_lz4(&ext->lz4_ctx, lod->realloc);
    }
    
    lod->__ext = NULL;
    lod->realloc = NULL;
}
