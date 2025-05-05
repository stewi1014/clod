/**
 * dh.h
 * 
 * Methods for dealing with DH LODs.
 */

#pragma once
#include <stdint.h>

#include "anvil.h"


struct dh_db;
struct dh_db *dh_db_open(char *path);
void dh_db_close(struct dh_db *);


typedef uint64_t dh_datapoint;

#define dh_datapoint_blocklight_mask (0xF000000000000000ULL)
#define dh_datapoint_skylight_mask   (0x0F00000000000000ULL)
#define dh_datapoint_min_y_mask      (0x00FFF00000000000ULL)
#define dh_datapoint_height_mask     (0x00000FFF00000000ULL)
#define dh_datapoint_id_mask         (0x00000000FFFFFFFFULL)

#define dh_datapoint_blocklight(v) ((dh_datapoint)(v) << 60)
#define dh_datapoint_skylight(v)   ((dh_datapoint)(v) << 56)
#define dh_datapoint_min_y(v)      ((dh_datapoint)(v) << 44)
#define dh_datapoint_height(v)     ((dh_datapoint)(v) << 32)
#define dh_datapoint_id(v)         ((dh_datapoint)(v) << 00)

#define dh_datapoint_set_blocklight(dp, v) ((dp) = ((dp) & 0x0FFFFFFFFFFFFFFFULL) | ((dh_datapoint)(v) << 60 & 0xF000000000000000ULL))
#define dh_datapoint_set_skylight(dp, v)   ((dp) = ((dp) & 0xF0FFFFFFFFFFFFFFULL) | ((dh_datapoint)(v) << 56 & 0x0F00000000000000ULL))
#define dh_datapoint_set_min_y(dp, v)      ((dp) = ((dp) & 0xFF000FFFFFFFFFFFULL) | ((dh_datapoint)(v) << 44 & 0x00FFF00000000000ULL))
#define dh_datapoint_set_height(dp, v)     ((dp) = ((dp) & 0xFFFFF000FFFFFFFFULL) | ((dh_datapoint)(v) << 32 & 0x00000FFF00000000ULL))
#define dh_datapoint_set_id(dp, v)         ((dp) = ((dp) & 0xFFFFFFFF00000000ULL) | ((dh_datapoint)(v) << 00 & 0x00000000FFFFFFFFULL))

#define dh_datapoint_get_blocklight(dp) ((dp) >> 60 & 0x000000000000000FULL)
#define dh_datapoint_get_skylight(dp)   ((dp) >> 56 & 0x000000000000000FULL)
#define dh_datapoint_get_min_y(dp)      ((dp) >> 44 & 0x0000000000000FFFULL)
#define dh_datapoint_get_height(dp)     ((dp) >> 32 & 0x0000000000000FFFULL)
#define dh_datapoint_get_id(dp)         ((dp) >> 00 & 0x00000000FFFFFFFFULL)

struct dh_lod {
    // ids in columns are indexes in mapping, thich contains
    // a string with biome, blockstate and blockstate properties.
    char **mapping;
    unsigned mapping_len; // number of valid elements in the array.
    unsigned mapping_cap; // total size of the allocated array. indexes above len have undefined values.

    dh_datapoint *column[64][64];
    uint16_t column_len[64][64];
    uint16_t column_cap[64][64];
};

/**
 * store a DH LOD in a DH database.
 */
int dh_db_store(struct dh_db *db, struct dh_lod *lod);

/**
 * creates the highest quality level DH LOD from chunk data.
 * 
 * make sure to pass a LOD that is no longer needed to the method if possible - it will reuse resources attached to it.
 * 
 * returns non-zero on failure.
 */
int dh_generate_from_chunks_ex(
    struct dh_lod **dh_lod,
    struct anvil_chunk chunks[4][4],
    void *(*realloc_f)(void*, size_t),
    void (*free_f)(void*)
);

#define DH_GENERATE_ERROR_ALLOCATE -1 // failed to allocate memory
#define DH_GENERATE_ERROR_MALFORMED -2 // chunk data is malformed

/**
 * creates the highest quality level DH LOD from chunk data.
 * 
 * make sure to pass a LOD that is no longer needed to the method if possible - it will reuse resources attached to it.
 * 
 * returns non-zero on failure.
 */
static inline
int dh_generate_from_chunks(
    struct dh_lod **dh_lod,
    struct anvil_chunk chunks[4][4]
) {
    return dh_generate_from_chunks_ex(dh_lod, chunks, NULL, NULL);
}

/**
 * creates the LOD for the next lower detail level.
 * 
 * make sure to pass a LOD that is no longer needed to the method if possible - it will reuse resources attached to it.
 * 
 * returns non-zero on failure.
 */
int dh_generate_from_lods_ex(
    struct dh_lod **dh_lod,
    struct dh_lod *lods[4],
    void *(*realloc_f)(void*, size_t),
    void (*free_f)(void*)
);

/**
 * creates the LOD for the next lower detail level.
 * 
 * make sure to pass a LOD that is no longer needed to the method if possible - it will reuse resources attached to it.
 * 
 * returns non-zero on failure.
 */
static inline
int dh_generate_from_lods(
    struct dh_lod **dh_lod,
    struct dh_lod *lods[4]
) {
    return dh_generate_from_lods_ex(dh_lod, lods, NULL, NULL);
}

#define DH_LOD_STATS_CLEAR (struct dh_lod_stats){0, 0, 0}

struct dh_lod_stats {
    size_t mem_metadata;
    size_t mem_unused;
    size_t mem_used;
    size_t num_lods;
};

void dh_lod_get_stats(
    struct dh_lod_stats *stats,
    struct dh_lod *lod
);

struct dh_lod_writer;
struct dh_lod_writer *dh_lod_writer_start(struct dh_lod *);
size_t dh_lod_writer_write(char *dest, size_t dest_size);
void dh_lod_writer_stop(struct dh_lod_writer *);

/**
 * frees resources accociated with the LOD.
 */
void dh_lod_free_ex(
    struct dh_lod *dh_lod,
    void (*free_f)(void*)
);

/**
 * frees resources accociated with the LOD.
 */
static inline
void dh_lod_free(
    struct dh_lod *dh_lod
) {
    dh_lod_free_ex(dh_lod, NULL);
}
