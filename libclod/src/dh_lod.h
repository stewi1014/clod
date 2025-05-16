#pragma once

#define LOD_GROW(cap, n) ((n) + (cap << 1) - (cap >> 1))
#define LOD_SHRINK(len, cap) ((cap))

#define DP_BLOCK_LIGHT_MASK   (0xF000000000000000ULL)
#define DP_SKY_LIGHT_MASK     (0x0F00000000000000ULL)
#define DP_MIN_Y_MASK         (0x00FFF00000000000ULL)
#define DP_HEIGHT_MASK        (0x00000FFF00000000ULL)
#define DP_ID_MASK            (0x00000000FFFFFFFFULL)

#define DP_BLOCK_LIGHT_SHIFT  (60)
#define DP_SKY_LIGHT_SHIFT    (56)
#define DP_MIN_Y_SHIFT        (44)
#define DP_HEIGHT_SHIFT       (32)
#define DP_ID_SHIFT           (00)

#define DP_BLOCK_LIGHT(dp) ((dp & DP_BLOCK_LIGHT_MASK ) >> DP_BLOCK_LIGHT_SHIFT)
#define DP_SKY_LIGHT(dp)   ((dp & DP_SKY_LIGHT_MASK   ) >> DP_SKY_LIGHT_SHIFT  )
#define DP_MIN_Y(dp)       ((dp & DP_MIN_Y_MASK       ) >> DP_MIN_Y_SHIFT      )
#define DP_HEIGHT(dp)      ((dp & DP_HEIGHT_MASK      ) >> DP_HEIGHT_SHIFT     )
#define DP_ID(dp)          ((dp & DP_ID_MASK          ) >> DP_ID_SHIFT         )

#define DP_SET_BLOCK_LIGHT(dp, v) ((dp &~ DP_BLOCK_LIGHT_MASK) | ((((uint64_t)(v)) << DP_BLOCK_LIGHT_SHIFT ) & DP_BLOCK_LIGHT_MASK))
#define DP_SET_SKY_LIGHT(dp, v)   ((dp &~ DP_SKY_LIGHT_MASK  ) | ((((uint64_t)(v)) << DP_SKY_LIGHT_SHIFT   ) & DP_SKY_LIGHT_MASK  ))
#define DP_SET_MIN_Y(dp, v)       ((dp &~ DP_MIN_Y_MASK      ) | ((((uint64_t)(v)) << DP_MIN_Y_SHIFT       ) & DP_MIN_Y_MASK      ))
#define DP_SET_HEIGHT(dp, v)      ((dp &~ DP_HEIGHT_MASK     ) | ((((uint64_t)(v)) << DP_HEIGHT_SHIFT      ) & DP_HEIGHT_MASK     ))
#define DP_SET_ID(dp, v)          ((dp &~ DP_ID_MASK         ) | ((((uint64_t)(v)) << DP_ID_SHIFT          ) & DP_ID_MASK         ))

static uint64_t dp_read(const char *data) {
    return
    (uint64_t)(uint8_t)data[0] << (7 * 8) |
    (uint64_t)(uint8_t)data[1] << (6 * 8) |
    (uint64_t)(uint8_t)data[2] << (5 * 8) |
    (uint64_t)(uint8_t)data[3] << (4 * 8) |
    (uint64_t)(uint8_t)data[4] << (3 * 8) |
    (uint64_t)(uint8_t)data[5] << (2 * 8) |
    (uint64_t)(uint8_t)data[6] << (1 * 8) |
    (uint64_t)(uint8_t)data[7] << (0 * 8) ;
}

static void dp_write(char *data, const uint64_t dp) {
    data[0] = (char)(uint8_t)(dp >> (7 * 8) & 0xFF);
    data[1] = (char)(uint8_t)(dp >> (6 * 8) & 0xFF);
    data[2] = (char)(uint8_t)(dp >> (5 * 8) & 0xFF);
    data[3] = (char)(uint8_t)(dp >> (4 * 8) & 0xFF);
    data[4] = (char)(uint8_t)(dp >> (3 * 8) & 0xFF);
    data[5] = (char)(uint8_t)(dp >> (2 * 8) & 0xFF);
    data[6] = (char)(uint8_t)(dp >> (1 * 8) & 0xFF);
    data[7] = (char)(uint8_t)(dp >> (0 * 8) & 0xFF);
}

int dh_compare_lod_pos(const void *, const void *);
int dh_compare_strings(const void *, const void *);

// TODO this is too complex
struct id_lookup {
    struct id_table {
        uint32_t *ids;
        size_t ids_cap;
        uint16_t air_block_state;
    } *sections;
    size_t sections_cap;
};

#define ID_LOOKUP_CLEAR (struct id_lookup){nullptr, 0}

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

dh_result dh_lod_reset(struct dh_lod *lod, struct dh_lod_ext **ext_ptr);
