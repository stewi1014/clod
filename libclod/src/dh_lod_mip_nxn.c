#include <stdint.h>
#include <dh.h>
#include "dh_lod.h"

#ifndef DH_MIP_NAME
#define DH_MIP_NAME _1x1
#define DH_MIP_LEVELS 1
#endif

#define SIZE (1<<DH_MIP_LEVELS)

#define __DH_CONCAT(prefix, suffix) prefix##suffix
#define DH_CONCAT(prefix, suffix) __DH_CONCAT(prefix, suffix)

#define read_check_column_len(data, end) ({\
    if (data + 2 <= end) return DH_ERR_MALFORMED;\
    int64_t value = (uint8_t)data[0] << 8 | (uint8_t)data[1];\
    if (data + 2 + value * 8 <= end) return DH_ERR_MALFORMED;\
    value;\
})

/**
 * I believe there are times when it's beneficial to have the code be more straightforward -
 * reflect the complexity of the task that it's performing without hiding it behind abstractions.
 * I believe this is one of those times.
 *
 * The reality is that mipping LODs in serialised format is a highly cross-coupled and complex operation,
 * and hiding it behind layers of abstraction makes it hard to see the bigger picture.
 * Understanding the bigger picture is, in my opinion, essential to writing half-decent code for this problem.
 *
 * I don't care if you spend the time understanding how this particular method works, or completely rewrite it from scratch.
 * Just make sure you understand the bigger picture.
 * Don't cut corners. Mipping is a complex task - give it the respect it deserves.
 */
dh_result DH_CONCAT(dh_lod_mip, DH_MIP_NAME)(
    struct dh_lod *lod,
    struct dh_lod **src
) {
    if (
        lod == NULL ||
        src == NULL ||
        dh_lods_same_min_y(src, SIZE * SIZE) == DH_LODS_NOT_SAME ||
        dh_lods_same_mip_level(src, SIZE * SIZE) == DH_LODS_NOT_SAME
    ) return DH_ERR_INVALID_ARGUMENT;

    struct dh_lod_ext *ext;
    dh_result res = dh_lod_ext_get(lod, &ext);
    if (res != DH_OK) return res;

    if (ext->big_buffer_cap > lod->lod_cap) {
        char *tmp = lod->lod_arr;
        lod->lod_arr = ext->big_buffer;
        ext->big_buffer = tmp;

        const auto tmp_cap = lod->lod_cap;
        lod->lod_cap = ext->big_buffer_cap;
        ext->big_buffer_cap = tmp_cap;
    }

    lod->x = 0;
    lod->z = 0;
    lod->height = 0;
    lod->min_y = dh_lods_same_min_y(src, SIZE * SIZE);
    lod->mip_level = dh_lods_same_mip_level(src, SIZE * SIZE) + DH_MIP_LEVELS;
    lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;
    lod->mapping_len = 0;
    lod->lod_len = 0;
    lod->has_data = false;

    // allocate everything on the stack. 83456 bytes in total for DH_MIP_LEVELS = 6.
    // I'm worried that this might be a bit too much for the stack.

    char *end[SIZE];
    char *z_cursor[SIZE];
    char *cursors[SIZE * SIZE];
    uint32_t *id_mapping[SIZE];      // source LOD -> destination LOD id lookup.
    uint16_t remaining[SIZE * SIZE]; // number of remaining datapoints + 1.
    uint16_t next_y[SIZE * SIZE];    // next datapoint transition.
    char *datapoints[SIZE * SIZE];   // current datapoint cursor.

    // loop over our LODs in x,
    // initialise the cursors for these LODs.
    for (int lod_x = 0; lod_x < SIZE; ++lod_x) {

        for (int lod_z = 0; lod_z < SIZE; ++lod_z) {
            struct dh_lod *src_lod = src[lod_x * SIZE + lod_z];

            end[lod_z] = src_lod->lod_arr + src_lod->lod_len;
            cursors[lod_z] = src_lod->lod_arr;

            res = dh_compress(src_lod, DH_DATA_COMPRESSION_UNCOMPRESSED, 0);
            if (res != DH_OK) return res;

            res = dh_lod_merge_mappings(
                lod, src_lod, &id_mapping[lod_z]
            );
            if (res != DH_OK) return res;
        }

    // no we have our LODs setup, iterate block-by-block on x.
    for (int sample_x = 0; sample_x < 64 / SIZE; sample_x++)
    for (
        int lod_z = 0
    ;
        lod_z < SIZE
    ;
        // store our position for when we come back to this lod.
        z_cursor[lod_z] =         cursors[SIZE * SIZE - 1] + 2 + 8 *
            read_check_column_len(cursors[SIZE * SIZE - 1], end[lod_z]),
        lod_z++
    ) {

        // load our previous position and derive our view from it.
        cursors[0] = z_cursor[lod_z];
        for (int cz = 1; cz < SIZE; cz++)
            cursors[cz] =             cursors[cz - 1] + 2 + 8 *
                read_check_column_len(cursors[cz - 1], end[lod_z]);

        for (int cx = 1; cx < SIZE; cx++) {
            cursors[cx * SIZE] = cursors[cx * SIZE - 1];
            for (int cz = SIZE - 1; cz < 64; cz++)
                cursors[cx * SIZE] =      cursors[cx * SIZE] + 2 + 8 *
                    read_check_column_len(cursors[cx * SIZE], end[lod_z]);

            for (int cz = 1; cz < SIZE; cz++) {
                cursors[cx * SIZE + cz] = cursors[cx * SIZE + cz - 1] + 2 + 8 *
                    read_check_column_len(cursors[cx * SIZE + cz - 1], end[lod_z]);
            }
        }

    for (
        int sample_z = 0
    ;
        sample_z < 64 / SIZE
    ;
        ({
        // move the cursors to the next z position
        for (int cx = 0; cx < SIZE; cx++) {
            cursors[cx * SIZE] =      cursors[cx * SIZE + SIZE - 1] + 2 + 8 *
                read_check_column_len(cursors[cx * SIZE + SIZE - 1], end[lod_z]);

            for (int cz = 1; cz < SIZE; cz++)
                cursors[cz * SIZE + cz] = cursors[cz * SIZE + cz - 1] + 2 + 8 *
                    read_check_column_len(cursors[cz * SIZE + cz - 1], end[lod_z]);
        }
        sample_z++;
        })
    ) {

        // get setup to iterate down the columns
        for (int i = 0; i < SIZE * SIZE; i++) {
            remaining[i] = read_check_column_len(cursors[i], end[lod_z]);
            if (remaining[i]) {
                const uint64_t datapoint = dp_read(cursors[i] + 1);
                next_y[i] = DP_MIN_Y(datapoint) + DP_HEIGHT(datapoint) + 1;
            } else {
                next_y[i] = 0;
            }
            datapoints[i] = nullptr;
        }

        char *cursor = lod->lod_arr + lod->lod_len + 2;
        uint_fast16_t min_y = 0, height;
        uint64_t last_datapoint = 0;

        last_datapoint = DP_SET_SKY_LIGHT(last_datapoint, 0xF);

        for (int i = 0; i < SIZE; i++) if (next_y[i] > min_y) min_y = next_y[i];

    while (({
        uint_fast16_t remaining_total = 0;
        for (int i = 0; i < SIZE; i++) if (next_y[i] == min_y && remaining[i] > 0) {
            if (remaining[i] == 1) {
                // transitioning out of the last datapoint
                datapoints[i] = nullptr;
                next_y[i] = 0;
                remaining[i]--;
                continue;
            }

            if (datapoints[i] == nullptr) {
                // transitioning into first datapoint
                datapoints[i] = cursors[i] + 2;
            } else {
                // transitioning from one datapoint to the next
                datapoints[i] += 8;
                remaining[i]--;
            }

            remaining_total += remaining[i];
            next_y[i] = DP_MIN_Y(dp_read(datapoints[i]));
        }

        uint_fast16_t next_min_y = 0;
        for (int i = 0; i < SIZE; i++) if (next_y[i] > next_min_y) next_min_y = next_y[i];

        height = min_y - next_min_y;
        min_y = next_min_y;
        remaining_total > 0;
    })) {
        uint32_t id_mode = 0;
        uint32_t total_block_light = 0, total_sky_light = 0;
        int id_count = 0;
        for (int i = 0; i < SIZE * SIZE; i++) if (datapoints[i] != nullptr) {
            int c = 0;
            const register uint64_t dp = dp_read(datapoints[i]);

            for (int j = 0; j < SIZE * SIZE; j++) if (datapoints[j] != nullptr)
                if (DP_ID(dp) == DP_ID(dp_read(datapoints[j])))
                    c++;

            total_block_light += DP_BLOCK_LIGHT(dp);
            total_sky_light += DP_SKY_LIGHT(dp);

            if (c > id_count) {
                id_count = c;
                id_mode = DP_ID(dp);
            }
        }

        if (DP_ID(last_datapoint) == id_mode) {
            last_datapoint = DP_SET_HEIGHT(last_datapoint, DP_HEIGHT(last_datapoint) + height);
            last_datapoint = DP_SET_MIN_Y(last_datapoint, min_y);

            continue;
        }

        if (DP_HEIGHT(last_datapoint) > 0) {
            last_datapoint = DP_SET_ID(last_datapoint, id_mapping[lod_z][DP_ID(last_datapoint)]);
            dp_write(cursor, last_datapoint);
            cursor += 8;
        }

        last_datapoint = 0;
        last_datapoint = DP_SET_SKY_LIGHT(last_datapoint, total_sky_light / (SIZE * SIZE));
        last_datapoint = DP_SET_BLOCK_LIGHT(last_datapoint, total_block_light / (SIZE * SIZE));
        last_datapoint = DP_SET_MIN_Y(last_datapoint, min_y);
        last_datapoint = DP_SET_HEIGHT(last_datapoint, height);
        last_datapoint = DP_SET_ID(last_datapoint, id_mode);
    }

        if (DP_HEIGHT(last_datapoint) > 0) {
            last_datapoint = DP_SET_ID(last_datapoint, id_mapping[lod_z][DP_ID(last_datapoint)]);
            dp_write(cursor, last_datapoint);
            cursor += 8;
        }

        const auto c = (cursor - 2 - lod->lod_arr - lod->lod_len) / 8;
        if (c > 0) lod->has_data = true;
        (lod->lod_arr + lod->lod_len)[0] = (char)(c >> (1 * 8) & 0xFF);
        (lod->lod_arr + lod->lod_len)[1] = (char)(c >> (0 * 8) & 0xFF);
        lod->lod_len = cursor - lod->lod_arr;

    }
    }
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

#undef read_check_column_len
