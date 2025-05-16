#include <stdint.h>

#include <dh.h>

#include "dh_lod.h"

#define __FUNC_NAME(prefix, suffix) prefix##suffix
#define FUNC_NAME(prefix, suffix) __FUNC_NAME(prefix, suffix)

dh_result FUNC_NAME(dh_lod_mip, NAME)(
    struct dh_lod *lod,
    struct dh_lod **lods
) {
    struct dh_lod_ext *ext;
    const dh_result res = dh_lod_reset(lod, &ext);
    if (res != DH_OK) return res;

    if (ext->big_buffer_cap > lod->lod_cap) {
        char *tmp = lod->lod_arr;
        lod->lod_arr = ext->big_buffer;
        ext->big_buffer = tmp;

        const auto tmp_cap = lod->lod_cap;
        lod->lod_cap = ext->big_buffer_cap;
        ext->big_buffer_cap = tmp_cap;
    }

#if SIZE == 16
    char *end[SIZE];
    uint32_t *id_mapping[SIZE];
    char *cursors[SIZE][SIZE * SIZE];
#else
    constexpr auto end_size = sizeof(char*) * SIZE;
    constexpr auto id_mapping_size = sizeof(uint32_t*) * SIZE;
    constexpr auto cursors_size = sizeof(char*) * SIZE * SIZE * SIZE;
    constexpr auto total_size = end_size + id_mapping_size + cursors_size;

    if (ext->big_buffer_cap < total_size) {
        char *new = lod->realloc(ext->big_buffer, total_size);
        if (new == nullptr) {
            return DH_ERR_ALLOC;
        }

        ext->big_buffer_cap = total_size;
        ext->big_buffer = new;
    }

    char *alloc_cursor = ext->big_buffer;

    auto end = (char**)alloc_cursor; alloc_cursor += end_size;
    auto id_mapping = (uint32_t)alloc_cursor; alloc_cursor += id_mapping_size;
    auto cursors = alloc_cursor; alloc_cursor += SIZE * SIZE * SIZE * sizeof(char*);



#endif

    for (int lod_x = 0; lod_x < SIZE; ++lod_x) {

    }

    return DH_OK;
}

/*
dh_result dh_from_one_below(
    struct dh_lod *lod,
    struct dh_lod **lods
) {
    struct dh_lod_ext *ext;
    dh_result result;

    for (int64_t i = 0; i < 4; i++)
        if ((result = dh_compress(lods[i], DH_DATA_COMPRESSION_UNCOMPRESSED, 0)) != DH_OK)
            return result;

    result = get_ext(lod, &ext);
    if (result != DH_OK)
        return result;

    lod->mapping_len = 0;
    lod->lod_len = 0;
    lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;
    lod->has_data = false;
    lod->mip_level = (*lods)->mip_level + 1;
    lod->min_y = (*lods)->min_y;
    lod->height = (*lods)->height;

    char *cursors[2][4];
    char *end[2];
    uint32_t *id_mapping[4];

    #define read_check_column_len(data, end) \
        ({\
            if (data + 2 <= end) return DH_ERR_MALFORMED;\
            int64_t value = (uint8_t)data[0] << 8 | (uint8_t)data[1];\
            if (data + 2 + value * 8 <= end) return DH_ERR_MALFORMED;\
            value;\
        })

    // loop over our LODs in x,
    // initialise the cursors for these LODs.
    for (int64_t lod_x = 0; lod_x < 2; lod_x++) {

        for (int64_t lod_z = 0; lod_z < 2; lod_z++) {
            end[lod_z] = lods[lod_x * 2 + lod_z]->lod_arr + lods[lod_x * 2 + lod_z]->lod_len;
            cursors[lod_z][2] = lods[lod_x * 2 + lod_z]->lod_arr;
            cursors[lod_z][3] = cursors[lod_z][2] + 2 + 8 * read_check_column_len(cursors[lod_z][2], end[lod_z]);
        }

    for (int64_t block_x = 0; block_x < 32; block_x++)

    // loop over our source LODs in z,
    // align our cursors to the new x position.
    for (int64_t lod_z = 0; lod_z < 2; lod_z++) {

        cursors[lod_z][0] = cursors[lod_z][2];
        cursors[lod_z][1] = cursors[lod_z][3];
        for (int64_t i = 0; i < 64; i++) cursors[lod_z][2] += 2 + 8 * read_check_column_len(cursors[lod_z][2], end[lod_z]);
        cursors[lod_z][3] = cursors[lod_z][2] + 2 + 8 * read_check_column_len(cursors[lod_z][2], end[lod_z]);

    // loop over source LOD z axis,
    // bringing our cursors along with us on the z axis.
    for (int64_t block_z = 0; block_z < 32; block_z++) {

        cursors[lod_z][0] = cursors[lod_z][1] + 2 + 8 * read_check_column_len(cursors[lod_z][1], end[lod_z]);
        cursors[lod_z][2] = cursors[lod_z][3] + 2 + 8 * read_check_column_len(cursors[lod_z][3], end[lod_z]);
        cursors[lod_z][1] = cursors[lod_z][0] + 2 + 8 * read_check_column_len(cursors[lod_z][0], end[lod_z]);
        cursors[lod_z][3] = cursors[lod_z][2] + 2 + 8 * read_check_column_len(cursors[lod_z][2], end[lod_z]);

        // TODO when optimising this later; No need for this to actually be two 4-length arrays.
        // column_index and next_y should fit in a single 128 bit register.
        // I'd like to see most of these 4-length loops replaced with a single instruction by the compiler.

        uint16_t dp_remaining[4];
        uint16_t next_y[4];
        char *datapoints[4] = {nullptr, nullptr, nullptr, nullptr};

    // iterate over every datapoint transition

    for (auto i = 0; i < 4; i++) {
        dp_remaining[i] = read_check_column_len(cursors[lod_z][i], end[lod_z]);

        if (dp_remaining[i])
            next_y[i] =
                DP_MIN_Y(dp_read(cursors[lod_z][i] + 2)) +
                DP_HEIGHT(dp_read(cursors[lod_z][i] + 2)) +
                1;
    }
    while (({
        uint_fast16_t y = 0;

        // find next transition
        for (int64_t i = 0; i < 4; i++) if (next_y[i]  > y) y = next_y[i];

        // run transition
        for (int64_t i = 0; i < 4; i++) if (next_y[i] == y) {
            // this column is transitioning.
            if (!dp_remaining[i]) {
                // no datapoints remaining in this column.
                // since
                continue;
            }

            if (dp_remaining[i] == 1) {
                // we're transitioning out of the last datapoint.
                datapoints[i] = nullptr;
                dp_remaining[i] = 0;
                next_y[i] = 0;
                continue;
            }

            if (datapoints[i] == nullptr) {
                // we're transitioning into the first datapoint in this column.
                datapoints[i] = cursors[lod_z][i] + 2;

            } else {
                // we're transitioning from one datapoint to the next.
                datapoints[i] += 8;
                dp_remaining[i]--;
            }

            next_y[i] = DP_MIN_Y(dp_read(datapoints[i]));
        }

        dp_remaining[0] ||
        dp_remaining[1] ||
        dp_remaining[2] ||
        dp_remaining[3] ;
    })) {
        uint32_t id_mode;
        if (
            (
                dp_remaining[1] && dp_remaining[2] &&
                DP_ID(dp_read(datapoints[1])) == DP_ID(dp_read(datapoints[2]))
            ) || (
                dp_remaining[1] && dp_remaining[3] &&
                DP_ID(dp_read(datapoints[1])) == DP_ID(dp_read(datapoints[3]))
            )
        ) {
            id_mode = DP_ID(dp_read(datapoints[1]));

        } else if (
            dp_remaining[2] && dp_remaining[3] &&
            DP_ID(dp_read(datapoints[2])) == DP_ID(dp_read(datapoints[3]))
        ) {
            id_mode = DP_ID(dp_read(datapoints[2]));

        } else for (int64_t i = 0; i < 4; i++) if (dp_remaining[0]) {
            id_mode = DP_ID(dp_read(datapoints[i]));
        }



    }
    }
    }
    }

    #undef read_check_column_len
    return DH_OK;
}
*/