#pragma once

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
