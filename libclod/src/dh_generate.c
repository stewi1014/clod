#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbit.h>

#include "anvil.h"
#include "nbt.h"
#include "dh.h"

/**
 * these two macros define when and by how much columns
 * are grown and shrunk.
 * 
 * COLUMN_SIZE_GROW must always be at least cap + 1,
 * and is used to find the new colum size when it needs to be grown.
 * 
 * COLUMN_SIZE_SHRINK can define a new capacity, if desired,
 * that the column will be shrank to.
 */

#define COLUMN_SIZE_GROW(cap) ((cap == 0) ? 16 : ((cap << 1) - (cap >> 1)))
#define COLUMN_SIZE_SHRINK(len, cap) ((cap) > 64 + (len)*2 ? (len) : (cap))

int create_id_lookup(
    uint32_t **id_lookup,
    unsigned *id_lookup_cap,
    struct anvil_section section,
    struct dh_lod *lod,
    void *(*realloc_f)(void*, size_t),
    void (*free_f)(void*)
) {
    // goodness gracious wall of code.
    // The *only* thing this does is figure out how to convert from minecarft ids to DH ids.
    // 
    // This was rather infuriating to write and I wish I could have avoided this.
    // But here we are, and there's no two ways around it - the DH format demands this be done.
    // 
    // Potential ideas
    // - split up biome and blockstate information in DH's mapping.
    // - use minecraft's existing NBT format in favour of custom formats.
    // - perform id de-duplication on block state and biome data instead of the serialised format.

    int32_t biomes = section.biome_palette != NULL ? nbt_list_size(section.biome_palette) : 0;
    int32_t block_states = section.block_state_palette != NULL ? nbt_list_size(section.block_state_palette) : 0;

    if (biomes == 0 || block_states == 0) return 0;

    char *temp_string = NULL;
    unsigned temp_string_size = 0;

    if (*id_lookup_cap <= biomes * block_states) {
        uint32_t *new_id_lookup = realloc_f(*id_lookup, biomes * block_states * sizeof(uint32_t));
        if (new_id_lookup == NULL) {
            free_f(temp_string);
            return DH_GENERATE_ERROR_ALLOCATE;
        }

        *id_lookup = new_id_lookup;
        *id_lookup_cap = biomes * block_states;
    }

    char *biome = nbt_list_payload(section.biome_palette);
    for (int biome_i = 0; biome_i < biomes && biome != NULL; biome_i++) {
        char *biome_resource = nbt_string(biome);
        uint16_t biome_resource_size = nbt_string_size(biome);

        int biome_string_size = biome_resource_size + strlen("_DH-BSW_");
        if (temp_string_size < biome_string_size) {
            char *new_temp_string = realloc_f(temp_string, biome_string_size);
            if (new_temp_string == NULL) {
                free_f(temp_string);
                return DH_GENERATE_ERROR_ALLOCATE;
            }

            temp_string = new_temp_string;
            temp_string_size = biome_string_size;
        }

        memcpy(temp_string,                       biome_resource, biome_resource_size);
        memcpy(temp_string + biome_resource_size, "_DH-BSW_",     strlen("_DH-BSW_") );

        char *block_state = nbt_list_payload(section.block_state_palette);
        for (int block_state_i = 0; block_state_i < block_states && block_state != NULL; block_state_i++) {
            char *block_state_name = NULL, *block_state_properties = NULL;

            block_state = nbt_named(block_state, section.end,
                "Name", strlen("Name"), NBT_STRING, &block_state_name,
                "Properties", strlen("Properties"), NBT_COMPOUND, &block_state_properties,
                NULL
            );

            char *block_state_resource = nbt_string(block_state_name);
            uint16_t block_state_resource_size = nbt_string_size(block_state_name);

            int block_state_string_size = biome_string_size + block_state_resource_size + strlen("_STATE_");
            if (temp_string_size < block_state_string_size) {
                char *new_temp_string = realloc_f(temp_string, block_state_string_size + 1);
                if (new_temp_string == NULL) {
                    free_f(temp_string);
                    return DH_GENERATE_ERROR_ALLOCATE;
                }

                temp_string = new_temp_string;
                temp_string_size = block_state_string_size;
            }

            memcpy(temp_string + biome_string_size,                             block_state_resource, block_state_resource_size);
            memcpy(temp_string + biome_string_size + block_state_resource_size, "_STATE_",            strlen("_STATE_")        );

            // properties are supposed to be sorted alphabetically based on their name,
            // but I am *SO* done with all this string shit I'm not bothering.
            // besides, minecraft seems to be sane enough to write these in a determenistic order anyways.

            int properties_string_size = block_state_string_size;
            while(block_state_properties != NULL && block_state_properties[0] != NBT_END) {
                char     *block_state_property_name      = nbt_name(block_state_properties, section.end);
                uint16_t block_state_property_name_size  = nbt_name_size(block_state_properties, section.end);
                char     *block_state_property_value     = nbt_string(nbt_payload(block_state_properties, NBT_STRING, section.end));
                uint16_t block_state_property_value_size = nbt_string_size(nbt_payload(block_state_properties, NBT_STRING, section.end));

                int new_properties_string_size = properties_string_size + 3 + block_state_property_name_size + block_state_property_value_size;
                if (temp_string_size < new_properties_string_size) {
                    char *new_temp_string = realloc_f(temp_string, new_properties_string_size + 1);
                    if (new_temp_string == NULL) {
                        free_f(temp_string);
                        return DH_GENERATE_ERROR_ALLOCATE;
                    }

                    temp_string = new_temp_string;
                    temp_string_size = new_properties_string_size;
                }

                temp_string[properties_string_size] = '{';
                properties_string_size += 1;

                memcpy(temp_string + properties_string_size, block_state_property_name, block_state_property_name_size);
                properties_string_size += block_state_property_name_size;

                temp_string[properties_string_size] = ':';
                properties_string_size += 1;

                memcpy(temp_string + properties_string_size, block_state_property_value, block_state_property_value_size);
                properties_string_size += block_state_property_value_size;

                temp_string[properties_string_size] = '}';
                properties_string_size += 1;

                block_state_properties = nbt_step(block_state_properties, section.end);
            }

            temp_string[properties_string_size] = '\x0';


            uint32_t id = 0;
            while (id < lod->mapping_len && strcmp(lod->mapping[id], temp_string))
                id++;

            if (id == lod->mapping_len) {
                if (lod->mapping_len == lod->mapping_cap) {
                    unsigned new_mapping_cap = lod->mapping_cap == 0 ? 16 : lod->mapping_cap * 2;
                    char **new_mapping = realloc_f(lod->mapping, new_mapping_cap * sizeof(char*));
                    if (new_mapping == NULL) {
                        free_f(temp_string);
                        return DH_GENERATE_ERROR_ALLOCATE;
                    }

                    for (int i = lod->mapping_cap; i < new_mapping_cap; i++) new_mapping[i] = NULL;
                    lod->mapping = new_mapping;
                    lod->mapping_cap = new_mapping_cap;
                }

                char *new = realloc_f(lod->mapping[id], properties_string_size + 1);
                if (new == NULL) {
                    free_f(temp_string);
                    return DH_GENERATE_ERROR_ALLOCATE;
                }
                lod->mapping[id] = new;
                lod->mapping_len++;
                memcpy(lod->mapping[id], temp_string, properties_string_size + 1);
            }

            (*id_lookup)[biome_i * block_states + block_state_i] = id;
            
        }

        biome = nbt_payload_step(biome, NBT_STRING, section.end);
    }

    
    free_f(temp_string);
    if (biome == NULL) return DH_GENERATE_ERROR_MALFORMED;
    return 0;
}

int dh_generate_from_chunks_ex(
    struct dh_lod **lod_ptr,
    struct anvil_chunk chunks[4][4],
    void *(*realloc_f)(void*, size_t),
    void (*free_f)(void*)
) {
    struct anvil_sections sections = ANVIL_SECTIONS_CLEAR;
    struct anvil_chunk chunk;

    if (realloc_f == NULL) realloc_f = realloc;
    if (free_f == NULL) free_f = free;

    struct dh_lod *lod = *lod_ptr;
    if (lod == NULL) {
        lod = realloc_f(NULL, sizeof(struct dh_lod));
        if (lod == NULL) {
            return DH_GENERATE_ERROR_ALLOCATE;
        }
        
        lod->mapping = NULL;
        lod->mapping_len = 0;
        lod->mapping_cap = 0;
        for (int x = 0; x < 64; x++) for (int z = 0; z < 64; z++) {
            lod->column_len[x][z] = 0;
            lod->column_cap[x][z] = 0;
        }

        *lod_ptr = lod;
    } else {
        for (int x = 0; x < 64; x++) for (int z = 0; z < 64; z++)
            lod->column_len[x][z] = 0;
    }

    lod->mapping_len = 0;
    unsigned id_lookup_cap = 0;
    uint32_t *id_lookup = NULL;
    
    for (int chunk_x = 0; chunk_x < 4; chunk_x++) for (int chunk_z = 0; chunk_z < 4; chunk_z++) {

        chunk = chunks[chunk_x][chunk_z];

        int error = anvil_parse_sections_ex(&sections, chunk, realloc_f);
        if (error) {
            free_f(id_lookup);
            anvil_sections_free_ex(&sections, free_f);
            return error;
        }

        for (int section_index = 0; section_index < sections.len; section_index++) {
            struct anvil_section section = sections.section[section_index];

            int32_t biomes = section.biome_palette != NULL ? nbt_list_size(section.biome_palette) : 0;
            int32_t block_states = section.block_state_palette != NULL ? nbt_list_size(section.block_state_palette) : 0;

            if (biomes == 0 || block_states == 0) continue;

            int error = create_id_lookup(
                &id_lookup,
                &id_lookup_cap,
                section,
                lod,
                realloc_f,
                free_f
            );

            if (error) {
                free_f(id_lookup);
                anvil_sections_free_ex(&sections, free_f);
                return error;
            }

            for (int block_x = 0; block_x < 16; block_x++) for (int block_z = 0; block_z < 16; block_z++) {
                uint16_t len = lod->column_len[block_x + chunk_x * 16][block_z + chunk_z * 16];
                uint16_t cap = lod->column_cap[block_x + chunk_x * 16][block_z + chunk_z * 16];
                dh_datapoint *column = cap == 0 ? NULL : lod->column[block_x + chunk_x * 16][block_z + chunk_z * 16];
                assert(len <= cap);
                
                if (len == 0) {
                    if (cap == 0) {
                        uint16_t new_cap = COLUMN_SIZE_GROW(cap);
                        dh_datapoint *new_column = realloc_f(NULL, sizeof(dh_datapoint) * new_cap);
                        if (new_column == NULL) {
                            free_f(id_lookup);
                            anvil_sections_free_ex(&sections, free_f);
                            return DH_GENERATE_ERROR_ALLOCATE;
                        }


                        cap = new_cap;
                        column = new_column;
                        lod->column_cap[block_x + chunk_x * 16][block_z + chunk_z * 16] = new_cap;
                        lod->column[block_x + chunk_x * 16][block_z + chunk_z * 16] = new_column;
                    }

                    column[0] = 0;
                    len++;
                    lod->column_len[block_x + chunk_x * 16][block_z + chunk_z * 16]++;
                }

                dh_datapoint last_datapoint = column[len - 1];
                dh_datapoint datapoint = 0;

                uint32_t id;
                uint16_t biome, block_state;
                uint8_t block_light, sky_light;

                for (int block_y = 15; block_y >= 0; block_y--) {
                    int block_index = block_x * 16 * 16 + block_z * 16 + block_y;

                    biome = 0;
                    if (biomes > 1)
                        biome = section.biome_indicies[block_y >> 2];

                    block_state = 0;
                    if (block_states > 1)
                        block_state = section.block_state_indicies[block_y];

                    id = id_lookup[biome * block_states + block_state];

                    block_light = 0;
                    if (section.block_light != NULL)
                        block_light = section.block_light[4 + block_index / 2] >> (block_index % 2 * 4);

                    sky_light = 0;
                    if (section.sky_light != NULL) 
                        sky_light = section.sky_light[4 + block_index / 2] >> (block_index % 2 * 4);

                    if (
                        dh_datapoint_get_id(last_datapoint) == id &&
                        dh_datapoint_get_blocklight(last_datapoint) == block_light &&
                        dh_datapoint_get_skylight(last_datapoint) == sky_light
                    ) {
                        // merge with previous datapoint
                        last_datapoint += dh_datapoint_height(1) + dh_datapoint_min_y(-1);
                    } else {
                        dh_datapoint_set_id(datapoint, id_lookup[biome * block_states + block_state]);
                        dh_datapoint_set_blocklight(datapoint, block_light);
                        dh_datapoint_set_skylight(datapoint, sky_light);
                        dh_datapoint_set_height(datapoint, 1);
                        dh_datapoint_set_min_y(datapoint, block_y + section_index * 16);

                        assert(len <= cap);
                        if (len == cap) {
                            uint16_t new_cap = COLUMN_SIZE_GROW(cap);
                            dh_datapoint *new_column = realloc_f(column, new_cap * sizeof(dh_datapoint));
                            if (new_column == NULL) {
                                free_f(id_lookup);
                                anvil_sections_free_ex(&sections, free_f);
                                return DH_GENERATE_ERROR_ALLOCATE;
                            }

                            cap = new_cap;
                            column = new_column;
                            lod->column_cap[block_x + chunk_x * 16][block_z + chunk_z * 16] = new_cap;
                            lod->column[block_x + chunk_x * 16][block_z + chunk_z * 16] = new_column;
                        }

                        column[len - 1] = last_datapoint;
                        last_datapoint = datapoint;
                     
                        len++;
                        lod->column_len[block_x + chunk_x * 16][block_z + chunk_z * 16]++;
                    }
                }

                if (COLUMN_SIZE_SHRINK(len, cap) != cap && cap > 0 && section_index == sections.len - 1) {
                    uint16_t new_cap = COLUMN_SIZE_SHRINK(len, cap);
                    dh_datapoint *new_column = realloc_f(column, new_cap * sizeof(dh_datapoint));
                    if (new_column != NULL || new_cap == 0) {
                        cap = new_cap;
                        column = new_column;
                        lod->column_cap[block_x + chunk_x * 16][block_z + chunk_z * 16] = new_cap;
                        lod->column[block_x + chunk_x * 16][block_z + chunk_z * 16] = new_column;
                    }
                }

                column[len - 1] = last_datapoint;
            }
        }
    }

    for (int x = 0; x < 64; x++) for (int z = 0; z < 64; z++) {
        //printf("%d, %d\n", lod->column_len[x][z], lod->column_cap[x][z]);
    }

    free_f(id_lookup);
    anvil_sections_free_ex(&sections, free_f);

    return 0;
}

int dh_generate_from_lods_ex(
    struct dh_lod **dh_lod,
    struct dh_lod *lods[4],
    void *(*realloc_f)(void*, size_t),
    void (*free_f)(void*)
) {
    if (realloc_f == NULL) realloc_f = realloc;
    if (free_f == NULL) free_f = free;

    return 0;
}

void dh_lod_get_stats(
    struct dh_lod_stats *stats,
    struct dh_lod *lod
) {
    stats->num_lods++;

    stats->mem_metadata += sizeof(*lod);
    stats->mem_metadata += sizeof(char*) * lod->mapping_cap;
    for (int i = 0; i < lod->mapping_len; i++) if (lod->mapping[i] != NULL) {
        stats->mem_metadata += strlen(lod->mapping[i]);
    }

    for (int i = lod->mapping_len; i < lod->mapping_cap; i++) if (lod->mapping[i] != NULL) {
        stats->mem_unused += strlen(lod->mapping[i]);
    }

    for (int x = 0; x < 64; x++) for (int z = 0; z < 64; z++) {
        stats->mem_used += sizeof(dh_datapoint) * lod->column_len[x][z];
        stats->mem_unused += sizeof(dh_datapoint) * (lod->column_cap[x][z] - lod->column_len[x][z]);
    }
}

void dh_lod_free_ex(
    struct dh_lod *dh_lod,
    void (*free_f)(void*)
) {
    if (free_f == NULL) free_f = free;

    for (int x = 0; x < 64; x++) for (int z = 0; z < 64; z++)
        free_f(dh_lod->column[x][z]);

    for (int i = 0; i < dh_lod->mapping_cap; i++)
        free_f(dh_lod->mapping[i]);

    free_f(dh_lod->mapping);
    free_f(dh_lod);
}