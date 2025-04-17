#include <stdio.h>

typedef uint64_t CLOD_DataPoint;

struct CLOD_Section {
    int x, y;
    unsigned char detail_level;

    CLOD_DataPoint data_points[64][64];
};

/**
 * WorldParsing
 */

struct CLOD_WorldReader;
struct CLOD_WorldReader *CLOD_readWorld(char *path);
bool CLOD_readSection(struct CLOD_WorldReader*, struct CLOD_Section*);
void CLOD_freeWorld(struct CLOD_WorldReader*);

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

struct CLOD_SerialiseCtx;
struct CLOD_SerialiseCtx *CLOD_createSerialiseCtx();
void CLOD_serialiseSection(struct CLOD_SerialiseCtx *buffers, struct CLOD_Section* section, FILE out);
void CLOD_freeSerialiseCtx(struct CLOD_SerialiseCtx*);

struct CLOD_DeserialiseCtx;
struct CLOD_DeserialiseCtx *CLOD_createDeserialiseCtx();
void CLOD_deserialiseSection(struct CLOD_DeserialiseCtx *buffers, struct CLOD_Section* section, FILE in);
void CLOD_freeDeserialiseCtx(struct CLOD_DeserialiseCtx*);
