#include <stdio.h>
#include <stdint.h>

typedef uint64_t CLOD_DataPoint;

struct CLOD_Section {
    int section_x, section_y;
    unsigned char detail_level;

    CLOD_DataPoint *data_points[64][64];
};

/**
 * WorldParsing
 */

struct CLOD_WorldReader;
struct CLOD_WorldReader *CLOD_readWorld(char*);
bool CLOD_readSection(struct CLOD_WorldReader*, struct CLOD_Section*);
void CLOD_freeWorld(struct CLOD_WorldReader*);

/**
 * LOD storage
 */

struct CLOD_Store;

struct CLOD_Store {
    void (*save_section)(struct CLOD_Store *self, struct CLOD_Section *section);
    void (*close)(struct CLOD_Store *self);
};

struct CLOD_Store *CLOD_open_sqlite3(char *path);
struct CLOD_Store *CLOD_open_postgres(char *url);

/**
 * Serialisation & Deserialisation - HowTo
 * 
 * A CLOD_SerialiseCtx and CLOD_DeserialiseCtx are reqired for serialisation and deserialisation respectively.
 * Use CLOD_createSerialiseCtx(), CLOD_createDeserialiseCtx(), CLOD_freeSerialiseCtx() and CLOD_freeDeserialiseCtx() to create/free this resource.
 * They hold buffers and temporary data used internally by serialisation and deserialisation methods,
 * and are intended to allow buffer reuse across operations - avoiding reallocation and regeneration.
 * The suggested usage is to create one for each thread and re-use it for every operation.
 * 
 * 
 */

enum CLOD_CompressionAlgo {
    CLOD_ZSTD,
    CLOD_LZMA,
    CLOD_LZ4,
};

struct CLOD_ZSTD_Options {
    enum CLOD_CompressionAlgo algo;
    int level;
};
#define CLOD_ZSTD_OPTIONS_DEFAULT {CLOD_ZSTD, 3}

struct CLOD_LZMA_Options {
    enum CLOD_CompressionAlgo algo;
    int level;
};
#define CLOD_LZMA_OPTIONS_DEFAULT {CLOD_LZMA, 6}

struct CLOD_LZ4_Options {
    enum CLOD_CompressionAlgo algo;
    int acceleration;
};
#define CLOD_LZ4_OPTIONS_DEFAULT {CLOD_LZ4, 1}

union CLOD_CompressionOptions {
    enum CLOD_CompressionAlgo algo;
    
    struct CLOD_ZSTD_Options zstd;
    struct CLOD_LZMA_Options lzma;
    struct CLOD_LZ4_Options lz4;
};

struct CLOD_SerialiseCtx;
struct CLOD_SerialiseCtx *CLOD_createSerialiseCtx();
int CLOD_serialiseSection(struct CLOD_SerialiseCtx*, struct CLOD_Section*, FILE*, union CLOD_CompressionOptions);
void CLOD_freeSerialiseCtx(struct CLOD_SerialiseCtx*);

struct CLOD_DeserialiseCtx;
struct CLOD_DeserialiseCtx *CLOD_createDeserialiseCtx();
int CLOD_deserialiseSection(struct CLOD_DeserialiseCtx*, struct CLOD_Section*, FILE*);
void CLOD_freeDeserialiseCtx(struct CLOD_DeserialiseCtx*);
