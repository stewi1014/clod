/**
 * dh.h
 * 
 * Methods for dealing with DH LODs.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>

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

#define DH_LOD_CLEAR (struct dh_lod) {0, 0, -64, 0, NULL, 0, 0, NULL, 0, 0, NULL, NULL}

struct dh_lod {
    int64_t x;                          // x position. based on the first chunk's position.
    int64_t z;                          // z position. based on the first chunk's position.
    int64_t min_y;                      // bottom of the LOD - y positions in LOD are relative to this.
    int64_t detail_level;               // detail level.

    char  **mapping_arr;                // id to biome, blockstate and blockstate properties mapping.
    int64_t mapping_len;                // size of the mapping.
    int64_t mapping_cap;                // size of allocated mapping.

    char   *lod_arr;                    // lod data in serialised form.
    int64_t lod_len;                    // length of serialised lod data.
    int64_t lod_cap;                    // size of allocated array.

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
