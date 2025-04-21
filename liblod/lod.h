#include <stdio.h>
#include <stdint.h>

typedef uint64_t LOD_DataPoint;

struct LOD_Section {
    int section_x, section_y;
    unsigned char detail_level;

    LOD_DataPoint *data_points[64][64];
};

/**
 * LOD storage
 */

struct LOD_Store;

struct LOD_Store {
    void (*save_section)(struct LOD_Store *self, struct LOD_Section *section);
    void (*close)(struct LOD_Store *self);
};

struct LOD_Store *LOD_open_DH_sqlite3(char *path);
struct LOD_Store *LOD_open_postgres(char *url);
struct LOD_Store *LOD_open_mysql(char *url);
struct LOD_Store *LOD_open_mariadb(char *url);

/**
 * Serialisation & Deserialisation - HowTo
 * 
 * A LOD_SerialiseCtx and LOD_DeserialiseCtx are reqired for serialisation and deserialisation respectively.
 * Use LOD_createSerialiseCtx(), LOD_createDeserialiseCtx(), LOD_freeSerialiseCtx() and LOD_freeDeserialiseCtx() to create/free this resource.
 * They hold buffers and temporary data used internally by serialisation and deserialisation methods,
 * and are intended to allow buffer reuse across operations - avoiding reallocation and regeneration.
 * The suggested usage is to create one for each thread and re-use it for every operation.
 * 
 * 
 */

enum LOD_CompressionAlgo {
    LOD_ZSTD,
    LOD_LZMA,
    LOD_LZ4,
};

struct LOD_ZSTD_Options {
    enum LOD_CompressionAlgo algo;
    int level;
};
#define LOD_ZSTD_OPTIONS_DEFAULT {LOD_ZSTD, 3}

struct LOD_LZMA_Options {
    enum LOD_CompressionAlgo algo;
    int level;
};
#define LOD_LZMA_OPTIONS_DEFAULT {LOD_LZMA, 6}

struct LOD_LZ4_Options {
    enum LOD_CompressionAlgo algo;
    int acceleration;
};
#define LOD_LZ4_OPTIONS_DEFAULT {LOD_LZ4, 1}

union LOD_CompressionOptions {
    enum LOD_CompressionAlgo algo;
    
    struct LOD_ZSTD_Options zstd;
    struct LOD_LZMA_Options lzma;
    struct LOD_LZ4_Options lz4;
};

struct LOD_SerialiseCtx;
struct LOD_SerialiseCtx *LOD_createSerialiseCtx();
int LOD_serialiseSection(struct LOD_SerialiseCtx*, struct LOD_Section*, FILE*, union LOD_CompressionOptions);
void LOD_freeSerialiseCtx(struct LOD_SerialiseCtx*);

struct LOD_DeserialiseCtx;
struct LOD_DeserialiseCtx *LOD_createDeserialiseCtx();
int LOD_deserialiseSection(struct LOD_DeserialiseCtx*, struct LOD_Section*, FILE*);
void LOD_freeDeserialiseCtx(struct LOD_DeserialiseCtx*);
