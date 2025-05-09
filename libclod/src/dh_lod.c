#include <stdlib.h>
#include <anvil.h>
#include <nbt.h>

#include "dh.h"

#define LOD_GROW(cap, n) ((cap == 0) ? (n) + 128 * 1024 : (n) + (cap << 1) - (cap >> 1))
#define LOD_SHRINK(len, cap) ((cap) > (len) * 3 && (len) > (cap>>4) ? (len) : (cap))

#define DH_DATAPOINT_LIGHT_MASK  (0xFF00000000000000ULL)
#define DH_DATAPOINT_MIN_Y_MASK  (0x00FFF00000000000ULL)
#define DH_DATAPOINT_HEIGHT_MASK (0x00000FFF00000000ULL)
#define DH_DATAPOINT_ID_MASK     (0x00000000FFFFFFFFULL)

#define DH_DATAPOINT_LIGHT_SHIFT  (56)
#define DH_DATAPOINT_MIN_Y_SHIFT  (44)
#define DH_DATAPOINT_HEIGHT_SHIFT (32)
#define DH_DATAPOINT_ID_SHIFT     (00)

struct id_lookup {
    struct id_table {
        uint32_t *ids;
        size_t ids_cap;
    } *sections;
    size_t sections_cap;
};

#define ID_LOOKUP_CLEAR (struct id_lookup){NULL, 0}


// this just so happens to be exactly 256 bytes large lol!
struct dh_lod_ext {
    char *temp_string;
    size_t temp_string_cap;

    char **temp_array;
    size_t temp_array_cap;

    char *temp_buffer;
    size_t temp_buffer_cap;

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
void increment_le_uint16_t(char *ptr) {
    uint16_t value = (uint8_t)ptr[0] | ((uint8_t)ptr[1] << 8);
    value++;
    ptr[0] = (char)(value & 0xFF);
    ptr[1] = (char)((value >> 8) & 0xFF);
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

        for (int64_t i = 0; i < 4; i++) {
            ext->sections[i] = ANVIL_SECTIONS_CLEAR;
            ext->id_lookup[i] = ID_LOOKUP_CLEAR;
        }

        lod->__ext = ext;
    }

    lod->mapping_len = 0;
    lod->lod_len = 0;
    lod->detail_level = 0;
    lod->x = chunks->chunk_x / 4;
    lod->z = chunks->chunk_z / 4;

    cursor = lod->lod_arr;

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

                uint64_t last_datapoint = (sections->len * 16) << DH_DATAPOINT_MIN_Y_SHIFT;

                ensure_buffer(2 + 8);
                size_t column_len_off = lod->lod_len;
                cursor = lod->lod_arr + lod->lod_len;
                cursor[0] = 0;
                cursor[1] = 0;
                lod->lod_len += 2;

                for (int64_t section_index = sections->len - 1; section_index >= 0; section_index--){
                    struct anvil_section *section = &sections->section[section_index];

                    if (section->biome_palette == NULL || section->block_state_palette == NULL)
                        continue;

                    uint32_t *id_table = id_lookup->sections[section_index].ids;

                    int32_t biome_count = nbt_list_size(section->biome_palette);
                    int32_t block_state_count = nbt_list_size(section->block_state_palette);

                    for (int64_t block_y = 15; block_y >= 0; block_y--){

                        unsigned biome = 0;
                        if (biome_count > 1) 
                            biome = section->biome_indicies[(block_x / 4) * 4 * 4 + (block_z / 4) * 4 + (block_y / 4)];

                        unsigned block_state = 0;
                        if (block_state_count > 1)
                            block_state = section->block_state_indicies[block_x * 16 * 16 + block_z * 16 + block_y];

                        assert(biome < biome_count);
                        assert(block_state < block_state_count);

                        uint32_t id = id_table[biome * block_state_count + block_state];

                        uint16_t light = 0;
                        if (section->block_light != NULL)
                            light |= section->block_light[(block_x * 16 * 16 + block_z * 16 + block_y) / 2] >> ((block_y & 1) * 4);
    
                        if (section->sky_light != NULL)
                            light |= section->sky_light[(block_x * 16 * 16 + block_z * 16 + block_y) / 2] >> (~(block_y & 1) * 4);
                
                        if (
                            ((last_datapoint & DH_DATAPOINT_ID_MASK) >> DH_DATAPOINT_ID_SHIFT) == id &&
                            ((last_datapoint & DH_DATAPOINT_LIGHT_MASK) >> DH_DATAPOINT_LIGHT_SHIFT) == light
                        ) {
                            last_datapoint += ((uint64_t)1 << DH_DATAPOINT_HEIGHT_SHIFT) + ((uint64_t)-1 << DH_DATAPOINT_MIN_Y_SHIFT);
                        } else {

                            ensure_buffer(8);
                            char *cursor = lod->lod_arr + lod->lod_len;
                            cursor[0] = (last_datapoint >> (0 * 8)) & 0xFF;
                            cursor[1] = (last_datapoint >> (1 * 8)) & 0xFF;
                            cursor[2] = (last_datapoint >> (2 * 8)) & 0xFF;
                            cursor[3] = (last_datapoint >> (3 * 8)) & 0xFF;
                            cursor[4] = (last_datapoint >> (4 * 8)) & 0xFF;
                            cursor[5] = (last_datapoint >> (5 * 8)) & 0xFF;
                            cursor[6] = (last_datapoint >> (6 * 8)) & 0xFF;
                            cursor[7] = (last_datapoint >> (7 * 8)) & 0xFF;

                            last_datapoint =
                                (uint64_t)light                           << DH_DATAPOINT_LIGHT_SHIFT  |
                                (uint64_t)(section_index * 16 + block_y)  << DH_DATAPOINT_MIN_Y_SHIFT  |
                                (uint64_t)1                               << DH_DATAPOINT_HEIGHT_SHIFT |
                                (uint64_t)id                              << DH_DATAPOINT_ID_SHIFT     ;

                            lod->lod_len += 8;
                            increment_le_uint16_t(lod->lod_arr + column_len_off);
                        }
                    }
                }

                ensure_buffer(8);
                char *cursor = lod->lod_arr + lod->lod_len;
                cursor[0] = (last_datapoint >> (0 * 8)) & 0xFF;
                cursor[1] = (last_datapoint >> (1 * 8)) & 0xFF;
                cursor[2] = (last_datapoint >> (2 * 8)) & 0xFF;
                cursor[3] = (last_datapoint >> (3 * 8)) & 0xFF;
                cursor[4] = (last_datapoint >> (4 * 8)) & 0xFF;
                cursor[5] = (last_datapoint >> (5 * 8)) & 0xFF;
                cursor[6] = (last_datapoint >> (6 * 8)) & 0xFF;
                cursor[7] = (last_datapoint >> (7 * 8)) & 0xFF;

                lod->lod_len += 8;
                increment_le_uint16_t(lod->lod_arr + column_len_off);

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

char *dh_lod_serialise_mapping(
    struct dh_lod *lod,
    size_t *nbytes
) {
    struct dh_lod_ext *ext = lod->__ext;
    *nbytes = 0;

    #define ensure_buffer(n) \
        if (ext->temp_buffer_cap < (n) + (*nbytes)) {\
            size_t new_cap = (n) + ext->temp_buffer_cap * 2;\
            char *new = lod->realloc(ext->temp_buffer, new_cap);\
            if (new == NULL) {\
                return NULL;\
            }\
            ext->temp_buffer_cap = new_cap;\
            ext->temp_buffer = new;\
        }\

    ensure_buffer(2);
    ext->temp_buffer[(*nbytes)++] = (lod->mapping_len >> 0) & 0xFF;
    ext->temp_buffer[(*nbytes)++] = (lod->mapping_len >> 8) & 0xFF;
    
    for (int64_t i = 0; i < lod->mapping_len; i++) {
        size_t size = strlen(lod->mapping_arr[i]);
        if (size > UINT16_MAX) {
            return NULL;
        }

        ensure_buffer(2 + size);
        ext->temp_buffer[(*nbytes)++] = (size >> 0) & 0xFF;
        ext->temp_buffer[(*nbytes)++] = (size >> 8) & 0xFF;
        memcpy(&ext->temp_buffer[*nbytes], lod->mapping_arr[i], size);
        *nbytes += size;
    }

    #undef ensure_buffer
    
    return ext->temp_buffer;
}

void dh_lod_free(
    struct dh_lod *lod
) {
    if (lod == NULL) return;
    if (lod->realloc == NULL) return;

    for (int64_t i = 0; i < lod->mapping_len; i++) lod->realloc(lod->mapping_arr[i], 0);
    lod->realloc(lod->mapping_arr, 0);
    lod->mapping_len = 0;

    lod->realloc(lod->lod_arr, 0);
    lod->lod_len = 0;
    lod->lod_cap = 0;

    struct dh_lod_ext *ext = lod->__ext;
    if (ext != NULL) {
        if (ext->temp_string != NULL) lod->realloc(ext->temp_string, 0);
        if (ext->temp_array != NULL) lod->realloc(ext->temp_array, 0);
        if (ext->temp_buffer != NULL) lod->realloc(ext->temp_buffer, 0);

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
    }
    
    lod->__ext = NULL;
    lod->realloc = NULL;
}
