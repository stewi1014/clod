/**
 * dh.h
 * 
 * Methods for dealing with DH LODs.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <anvil.h>

//================//
// LOD Generation //
//================//

typedef enum dh_result {
    DH_OK,                      // operation completed successfully.
    DH_ERR_INVALID_ARGUMENT,    // an invalid argument was given.
    DH_ERR_ALLOC,               // memory allocation failure.
    DH_ERR_MALFORMED            // data is malformed
} dh_result;

#define DH_DATA_COMPRESSION_UNCOMPRESSED 0
#define DH_DATA_COMPRESSION_LZ4 1
#define DH_DATA_COMPRESSION_Z_STD 2
#define DH_DATA_COMPRESSION_LZMA2 3

#define DH_LOD_CLEAR (struct dh_lod) {\
    0, 0, -64, 0, DH_DATA_COMPRESSION_UNCOMPRESSED, \
    NULL, 0, 0, \
    NULL, 0, 0, \
    true, \
    NULL, NULL\
}

struct dh_lod {
    int64_t x;                          // x position. based on the first chunk's position.
    int64_t z;                          // z position. based on the first chunk's position.
    int64_t min_y;                      // bottom of the LOD - y positions in LOD are relative to this.
    int64_t detail_level;               // detail level.
    int64_t compression_mode;           // type of compression used to compress the LOD.

    char  **mapping_arr;                // id to biome, blockstate and blockstate properties mapping.
    int64_t mapping_len;                // size of the mapping.
    int64_t mapping_cap;                // size of allocated mapping.

    char   *lod_arr;                    // lod data in serialised form.
    int64_t lod_len;                    // length of serialised lod data.
    int64_t lod_cap;                    // size of allocated array.

    bool has_data;                      // true if the LOD contains any non-empty columns.

    void *(*realloc)(void*, size_t);    // used to allocate and free memory. methods will set this if it is NULL.
    void *__ext;                        // internal usage.
};

/**
 * generates a DH LOD from chunk data.
 */
dh_result dh_from_chunks(
    struct anvil_chunk *chunks, // 4x4 array of chunks.
    struct dh_lod *lod          // destination LOD.
);

/**
 * generates the next lower quality DH LOD from higher quality LOD data.
 */
dh_result dh_from_lods(
    struct dh_lod *lods, // 2x2 array of source LODs.
    struct dh_lod *lod   // destination LOD.
);

/**
 * returns the mapping in serialised form.
 * 
 * the buffer is reused between calls.
 * 
 * it returns NULL on allocation failure or if a mapping string is > 2^16 - 1 bytes.
 */
char *dh_lod_serialise_mapping(struct dh_lod *lod, size_t *nbytes);

/**
 * converts the format from its current compression format to the requested format.
 * 
 * a compression level of .5 is expected to be a reasonable
 * compression level for all compression types,
 * and the number may be adjusted from 0 to 1 inclusive,
 * with 0 and 1 being the lowest and highest compression levels the mode supports.
 */
dh_result dh_compress(
    struct dh_lod *lod,
    int64_t compression_mode,
    double level
);

/**
 * frees resources associated with the lod and clears it.
 */
void dh_lod_free(
    struct dh_lod *lod
);


//==================//
// SQlite3 Database //
//==================//

struct dh_db;
struct dh_db *dh_db_open(char *path);
void dh_db_close(struct dh_db *db);
int dh_db_store(struct dh_db *db, struct dh_lod *lod);
