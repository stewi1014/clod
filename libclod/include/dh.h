/**
 * dh.h
 * 
 * Methods for dealing with DH LODs.
 */
#pragma once
#include <stddef.h>

#include <anvil.h>

//================//
// LOD Generation //
//================//

typedef enum dh_result {
    DH_INVALID,                 // never intentionally returned.
    DH_OK,                      // operation completed successfully.
    DH_ERR_INVALID_ARGUMENT,    // an invalid argument was given.
    DH_ERR_ALLOC,               // memory allocation failure.
    DH_ERR_MALFORMED,           // data is malformed.
    DH_ERR_UNSUPPORTED,         // the operation is currently unsupported.
    DH_ERR_COMPRESS             // compression or decompression failed.
} dh_result;

#define DH_DATA_COMPRESSION_UNCOMPRESSED 0
#define DH_DATA_COMPRESSION_LZ4 1
#define DH_DATA_COMPRESSION_ZSTD 2
#define DH_DATA_COMPRESSION_LZMA2 3

#define DH_LOD_CLEAR (struct dh_lod) {\
    0, 0, 0, 0, 0, DH_DATA_COMPRESSION_UNCOMPRESSED, \
    nullptr, 0, 0, \
    nullptr, 0, 0, \
    false, \
    nullptr, nullptr\
}

struct dh_lod {
    int64_t x;                          // x position. based on the first chunk's position.
    int64_t z;                          // z position. based on the first chunk's position.
    int64_t height;                     // height of the LOD.
    int64_t min_y;                      // bottom of the LOD - y positions in LOD are relative to this.
    int64_t mip_level;                  // mip/detail level. 0, 1, 2, 3... = 1, 2, 4, 8... block wide datapoints.
    int64_t compression_mode;           // type of compression used to compress the LOD.

    char  **mapping_arr;                // id to biome, block state and block state properties mapping.
    size_t mapping_len;                // size of the mapping.
    size_t mapping_cap;                // size of allocated mapping.

    char   *lod_arr;                    // lod data in serialised form.
    size_t lod_len;                    // length of serialised lod data.
    size_t lod_cap;                    // size of allocated array.

    bool has_data;                      // true if the LOD contains any non-empty columns.

    void *(*realloc)(void*, size_t);    // used to allocate and free memory. methods will set this if it is nullptr.
    void *__ext;                        // internal usage.
};

/**
 * generates a DH LOD from chunk data.
 */
dh_result dh_from_chunks(
    const struct anvil_chunk *chunks, // 4x4 array of chunks.
    struct dh_lod *lod          // destination LOD.
);

/**
 * creates a LOD with the given mip level using data from the given LODs.
 *
 * it does not currently support arbitrary generation, and instead
 * only supports 2x2, 4x4, 8x8, 16x16, 32x32 and 64x64 input lods of the same mip level,
 * to generate a +1,  +2,  +3,  +4,    +5    and +6 mip level LOD respectively.
 *
 * notably, 64x64 input lods is about a gigabyte of LOD data.
 * that is a truly insane amount of memory for this task.
 * generating a single LOD from more data than that in one go is unlikely to ever be supported.
 * instead, generation should be done in stages, and only relevant data kept in memory instead.
 *
 * LODs may be null or empty.
 * 
 * it will probably decompress the source LODs and not recompress them.
 */
dh_result dh_lod_mip(
    struct dh_lod *lod,   // destination LOD.
    int64_t mip_level,    // mip level to generate.
    struct dh_lod **lods, // source LODs.
    int64_t num_lods      // number of source LODs.
);

/**
 * returns the mapping in serialised form.
 * 
 * the buffer is reused between calls.
 * 
 * it returns nullptr on allocation failure or if a mapping string is > 2^16 - 1 bytes.
 */
dh_result dh_lod_serialise_mapping(
    const struct dh_lod *lod,
    char **out,
    size_t *n_bytes
);

/**
 * converts the format from its current compression format to the requested format,
 * it takes uncomrpessed as an argument and is the correct method to use for decompression.
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
struct dh_db *dh_db_open(const char *path);
void dh_db_close(struct dh_db *db);
int dh_db_store(const struct dh_db *db, struct dh_lod *lod);
