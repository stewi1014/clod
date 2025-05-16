#include <dh.h>
#include <nbt.h>
#include <stdlib.h>

#include "dh_lod.h"

int compare_tag_name(const void *tag1, const void *tag2) {
    size_t size = nbt_name_size((char*)tag1, nullptr);
    if (nbt_name_size((char*)tag2, nullptr) < size) {
        size = nbt_name_size((char*)tag2, nullptr);
    }

    const int n = strncmp(nbt_name((char*)tag1, nullptr), nbt_name((char*)tag2, nullptr), size);
    if (n) return n;
    if (nbt_name_size((char*)tag1, nullptr) > nbt_name_size((char*)tag2, nullptr)) return 1;
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
    const struct anvil_sections *sections,
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
            if (new == nullptr) return DH_ERR_ALLOC;\
            ext->temp_string = new;\
            ext->temp_string_cap = (off) + (src_len);\
        }\
        memcpy(ext->temp_string + (off), (src), (src_len));\
        (off) += (src_len);


    struct dh_lod_ext *ext = lod->__ext;

    if (id_table->sections_cap < sections->len) {
        const size_t new_cap = sections->len;
        struct id_table *new = lod->realloc(id_table->sections, new_cap * sizeof(*id_table->sections));
        if (new == nullptr) {
            return DH_ERR_ALLOC;
        }

        for (auto i = id_table->sections_cap; i < new_cap; i++) {
            new[i].ids_cap = 0;
            new[i].ids = nullptr;
        }

        id_table->sections = new;
        id_table->sections_cap = new_cap;
    }

    for (auto section_index = 0; section_index < sections->len; section_index++) {
        const struct anvil_section section = sections->section[section_index];

        if (section.biome_palette == nullptr || section.block_state_palette == nullptr)
            continue;

        const auto biomes = nbt_list_size(section.biome_palette);
        if (biomes > 64 || biomes < 0) {
            return DH_ERR_MALFORMED;
        }

        const auto block_states = nbt_list_size(section.block_state_palette);
        if (block_states > 4096 || block_states < 0) {
            return DH_ERR_MALFORMED;
        }

        if (id_table->sections[section_index].ids_cap < biomes * block_states) {
            const size_t new_cap = biomes * block_states;
            uint32_t *new = lod->realloc(
                id_table->sections[section_index].ids,
                new_cap * sizeof(*id_table->sections[section_index].ids)
            );

            if (new == nullptr) {
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
                char *block_state_name = nullptr, *block_state_property = nullptr;
                nbt_named(block_state, section.end,
                    "Name", strlen("Name"), NBT_STRING, &block_state_name,
                    "Properties", strlen("Properties"), NBT_COMPOUND, &block_state_property,
                    nullptr
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

                if (block_state_property != nullptr) {
                    int64_t property_count = 0;
                    while (block_state_property[0] == NBT_STRING) {
                        if (ext->temp_array_cap == property_count) {
                            char **new = lod->realloc(ext->temp_array, (2 * ext->temp_array_cap + 2) * sizeof(char*));
                            if (new == nullptr) {
                                return DH_ERR_ALLOC;
                            }
                            ext->temp_array = new;
                            ext->temp_array_cap = 2 * ext->temp_array_cap + 2;
                        }

                        ext->temp_array[property_count] = block_state_property;
                        block_state_property = nbt_step(block_state_property, section.end);
                        property_count++;
                    }

                    // every language has a sort method
                    // this is C's

                    qsort(ext->temp_array, property_count, sizeof(char*), compare_tag_name);

                    for (int property_index = 0; property_index < property_count; property_index++) {
                        block_state_property = ext->temp_array[property_index];

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
                while (id < lod->mapping_len && strcmp(lod->mapping_arr[id], ext->temp_string) != 0)
                    id++;

                if (id == lod->mapping_len) {
                    if (lod->mapping_len == lod->mapping_cap) {
                        const size_t new_cap = lod->mapping_cap == 0 ? 16 : lod->mapping_cap * 2;
                        char **new = lod->realloc(lod->mapping_arr, new_cap * sizeof(char*));
                        if (new == nullptr) {
                            return DH_ERR_ALLOC;
                        }
                        for (auto i = lod->mapping_cap; i < new_cap; i++) new[i] = nullptr;
                        lod->mapping_arr = new;
                        lod->mapping_cap = new_cap;
                    }

                    char *new = lod->realloc(lod->mapping_arr[id], block_state_string_size);
                    if (new == nullptr) {
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
    const struct anvil_chunk *chunks,  // 4x4 array of chunks.
    struct dh_lod *lod          // destination LOD.
) {
    if (chunks == nullptr || lod == nullptr) return DH_ERR_INVALID_ARGUMENT;

    struct dh_lod_ext *ext;
    const dh_result res = dh_lod_reset(lod, &ext);
    if (res != DH_OK) return res;

    lod->mip_level = 0;
    lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;
    lod->x = chunks->chunk_x / 4;
    lod->z = chunks->chunk_z / 4;
    lod->has_data = false;

    if (ext->big_buffer_cap > lod->lod_cap) {
        char *tmp = lod->lod_arr;
        lod->lod_arr = ext->big_buffer;
        ext->big_buffer = tmp;

        const auto tmp_cap = lod->lod_cap;
        lod->lod_cap = ext->big_buffer_cap;
        ext->big_buffer_cap = tmp_cap;
    }

    #define ensure_buffer(n) ({\
        if (lod->lod_cap < lod->lod_len + (n)) {\
            size_t new_cap = LOD_GROW(lod->lod_cap, n);\
            char *new = lod->realloc(lod->lod_arr, new_cap);\
            if (new == nullptr) {\
                return DH_ERR_ALLOC;\
            }\
            lod->lod_arr = new;\
            lod->lod_cap = new_cap;\
        }\
    })

    for (int64_t chunk_x = 0; chunk_x < 4; chunk_x++) {

        for (int64_t chunk_z = 0; chunk_z < 4; chunk_z++) {
            const int error = anvil_parse_sections_ex(
                &ext->sections[chunk_z],
                chunks[chunk_x * 4 + chunk_z],
                lod->realloc
            );

            if (error) {
                return DH_ERR_MALFORMED;
            }

            const dh_result result = add_mappings(
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

            const struct anvil_sections *sections = &ext->sections[chunk_z];
            const struct id_lookup *id_lookup = &ext->id_lookup[chunk_z];

            for (int64_t block_z = 0; block_z < 16; block_z++) {

                ensure_buffer(2 + 8 * sections->len * 16);

                const size_t column_len_off = lod->lod_len;
                char* cursor = lod->lod_arr + lod->lod_len;
                cursor[0] = 0;
                cursor[1] = 0;
                lod->lod_len += 2;

                if (
                    sections->status != nullptr && (
                    strlen("minecraft:full") != nbt_string_size(sections->status) ||
                    0 != strncmp("minecraft:full", nbt_string(sections->status), strlen("minecraft:full"))
                )) {
                    continue;
                }

                uint64_t last_datapoint =
                    0xFULL                  << DP_SKY_LIGHT_SHIFT |
                    (sections->len * 16)    << DP_MIN_Y_SHIFT     ;

                uint64_t this_datapoint = 0;

                uint64_t next_datapoint =
                    1ULL << DP_HEIGHT_SHIFT;

                for (int64_t section_index = sections->len - 1; section_index >= 0; section_index--){
                    const auto section = &sections->section[section_index];

                    if (section->biome_palette == nullptr || section->block_state_palette == nullptr)
                        continue;

                    const auto id_table = id_lookup->sections[section_index].ids;

                    const auto biome_count = nbt_list_size(section->biome_palette);
                    const auto block_state_count = nbt_list_size(section->block_state_palette);

                    for (int64_t block_y = 15; block_y >= 0; block_y--){
                        const int64_t index = block_y * 16 * 16 + block_z * 16 + block_x;

                        int32_t biome = 0;
                        if (biome_count > 1)
                            biome = section->biome_indicies[(block_y / 4) * 4 * 4 + (block_z / 4) * 4 + (block_x / 4)];

                        int32_t block_state = 0;
                        if (block_state_count > 1)
                            block_state = section->block_state_indicies[index];

                        assert(biome < biome_count);
                        assert(block_state < block_state_count);

                        this_datapoint = next_datapoint;

                        if (section->sky_light != nullptr) {
                            next_datapoint = DP_SET_SKY_LIGHT(
                                next_datapoint,
                                (section->sky_light[index / 2] >> ((index & 1) * 4)) & 0xF
                            );
                        }

                        if (section->block_light != nullptr) {
                            next_datapoint = DP_SET_BLOCK_LIGHT(
                                next_datapoint,
                                (section->block_light[index / 2] >> ((index & 1) * 4)) & 0xF
                            );
                        } else {
                            next_datapoint = DP_SET_BLOCK_LIGHT(next_datapoint, 0);
                        }

                        const auto id = id_table[biome * block_state_count + block_state];
                        if ((last_datapoint & DP_ID_MASK) >> DP_ID_SHIFT == id) {
                            last_datapoint +=
                                ((uint64_t)1 << DP_HEIGHT_SHIFT) +
                                ((uint64_t)-1 << DP_MIN_Y_SHIFT);

                            continue;
                        }

                        if ((last_datapoint & DP_HEIGHT_MASK) >> DP_HEIGHT_SHIFT > 0) {
                            lod->has_data = true;
                            cursor = lod->lod_arr + lod->lod_len;
                            cursor[0] = (char)((last_datapoint >> (7 * 8)) & 0xFF);
                            cursor[1] = (char)((last_datapoint >> (6 * 8)) & 0xFF);
                            cursor[2] = (char)((last_datapoint >> (5 * 8)) & 0xFF);
                            cursor[3] = (char)((last_datapoint >> (4 * 8)) & 0xFF);
                            cursor[4] = (char)((last_datapoint >> (3 * 8)) & 0xFF);
                            cursor[5] = (char)((last_datapoint >> (2 * 8)) & 0xFF);
                            cursor[6] = (char)((last_datapoint >> (1 * 8)) & 0xFF);
                            cursor[7] = (char)((last_datapoint >> (0 * 8)) & 0xFF);
                            lod->lod_len += 8;
                            increment_be_uint16_t(lod->lod_arr + column_len_off);
                        }

                        last_datapoint =
                            this_datapoint                                              |
                            (uint64_t)(section_index * 16 + block_y)  << DP_MIN_Y_SHIFT |
                            (uint64_t)id                              << DP_ID_SHIFT    ;
                    }
                }

                if ((last_datapoint & DP_HEIGHT_MASK) >> DP_HEIGHT_SHIFT > 0) {
                    lod->has_data = true;
                    cursor = lod->lod_arr + lod->lod_len;
                    cursor[0] = (char)((last_datapoint >> (7 * 8)) & 0xFF);
                    cursor[1] = (char)((last_datapoint >> (6 * 8)) & 0xFF);
                    cursor[2] = (char)((last_datapoint >> (5 * 8)) & 0xFF);
                    cursor[3] = (char)((last_datapoint >> (4 * 8)) & 0xFF);
                    cursor[4] = (char)((last_datapoint >> (3 * 8)) & 0xFF);
                    cursor[5] = (char)((last_datapoint >> (2 * 8)) & 0xFF);
                    cursor[6] = (char)((last_datapoint >> (1 * 8)) & 0xFF);
                    cursor[7] = (char)((last_datapoint >> (0 * 8)) & 0xFF);
                    lod->lod_len += 8;
                    increment_be_uint16_t(lod->lod_arr + column_len_off);
                }

            }
        }
    }

    const auto shrink_cap = LOD_SHRINK(lod->lod_len, lod->lod_cap);
    if (shrink_cap < lod->lod_cap) {
        char *new = lod->realloc(lod->lod_arr, shrink_cap);
        // don't really care if shrinking fails.
        if (new != nullptr) {
            lod->lod_arr = new;
            lod->lod_cap = shrink_cap;
        }
    }

    #undef ensure_buffer

    return DH_OK;
}