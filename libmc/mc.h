#pragma once

#include <stdio.h>
#include <time.h>

/**
 * Minecraft Data Library
 * 
 * Very simple library that just gets the job done without any faffing about.
 */

/**
 * NBT Parsing
 */

#define MC_NBT_UNPARSED -1
#define MC_NBT_END 0
#define MC_NBT_BYTE 1
#define MC_NBT_SHORT 2
#define MC_NBT_INT 3
#define MC_NBT_LONG 4
#define MC_NBT_FLOAT 5
#define MC_NBT_DOUBLE 6
#define MC_NBT_BYTE_ARRAY 7
#define MC_NBT_STRING 8
#define MC_NBT_LIST 9
#define MC_NBT_COMPOUND 10
#define MC_NBT_INT_ARRAY 11
#define MC_NBT_LONG_ARRAY 12

#define MC_NBT_NEW ((struct MC_nbt){.type = MC_NBT_END, .payload = {.unparsed.data = NULL}})

struct MC_nbt;
union MC_nbt_payload;

union MC_nbt_payload {
    struct { char *data; int size_v; } unparsed;  // MC_NBT_UNPARSED
    //void           // MC_NBT_END
    char byte_v;     // MC_NBT_BYTE
    short short_v;   // MC_NBT_SHORT
    int int_v;       // MC_NBT_INT
    long long_v;     // MC_NBT_LONG
    float float_v;   // MC_NBT_FLOAT
    double double_v; // MC_NBT_DOUBLE
    struct { char                 *array_v; int            size_v;              } byte_array; // MC_NBT_BYTE_ARRAY
    struct { char                 *array_v; unsigned short size_v;              } string;     // MC_NBT_STRING
    struct { union MC_nbt_payload *array_v; int            size_v; char type_v; } list;       // MC_NBT_LIST
    struct { struct MC_nbt        *array_v; unsigned long  size_v;              } compound;   // MC_NBT_COMPOUND
    struct { int                  *array_v; int            size_v;              } int_array;  // MC_NBT_INT_ARRAY
    struct { long                 *array_v; int            size_v;              } long_array; // MC_NBT_LONG_ARRAY
};

struct MC_nbt {
    union MC_nbt_payload payload;
    char *name;
    unsigned short name_size;
    char type;
    int cap;
};

/**
 * MC_nbt_parse parses the given NBT data to the given depth,
 * and returns the byte after the end of the NBT data.
 * 
 * if nbt is not null, it must be zeroed or the result of a previous call to MC_nbt_parse!!
 * use MC_NBT_NEW to initialise a zeroed nbt.
 * 
 * It retains references to the NBT data, either to avoid copying,
 * or to facilitate re-parsing a partially populated nbt.
 * 
 * returns null if data is null.
 */
char *MC_nbt_parse(struct MC_nbt*, char *data, int depth);
void MC_nbt_free(struct MC_nbt*);

/**
 * World Reading
 */

// filename for the region file for the given location.
#define MC_region_filename(region_x, region_z) \
    ("r." region_x "." region_z ".mca") 

// offset (in 4096-byte sectors) of the given chunk.
#define MC_region_chunk_offset(region_file, chunk_x, chunk_z) (\
    ((unsigned int)(unsigned char)region_file[0+4*(((chunk_x)&31)+((chunk_z)&31)*32)]<<16) +\
    ((unsigned int)(unsigned char)region_file[1+4*(((chunk_x)&31)+((chunk_z)&31)*32)]<<8) +\
    ((unsigned int)(unsigned char)region_file[2+4*(((chunk_x)&31)+((chunk_z)&31)*32)]))

// number of 4096-byte sectors that the chunk occupies.
#define MC_region_chunk_sectors(region_file, chunk_x, chunk_z) \
    ((unsigned char)region_file[3+4*(((chunk_x)&31)+((chunk_z)&31)*32)])

// last modification time (in epoch seconds) of the chunk.
#define MC_region_chunk_mtime(region_file, chunk_x, chunk_z) \
    ((time_t)(unsigned char)region_file[0+4096+4*(((chunk_x)&31)+((chunk_z)&31)*32)]<<24) +\
    ((time_t)(unsigned char)region_file[1+4096+4*(((chunk_x)&31)+((chunk_z)&31)*32)]<<16) +\
    ((time_t)(unsigned char)region_file[2+4096+4*(((chunk_x)&31)+((chunk_z)&31)*32)]<<8) +\
    ((time_t)(unsigned char)region_file[3+4096+4*(((chunk_x)&31)+((chunk_z)&31)*32)])

// pointer to the chunk.
#define MC_region_get_chunk(region_file, chunk_x, chunk_z) (\
    MC_region_chunk_offset(region_file, chunk_x, chunk_z) > 1\
    && MC_region_chunk_sectors(region_file, chunk_x, chunk_z) > 0\
    ? region_file + 4096 * MC_region_chunk_offset(region_file, chunk_x, chunk_z)\
    : NULL)

/**
 * Chunk Data Reading
 */

#define MC_CHUNK_COMPRESSION_GZIP 1
#define MC_CHUNK_COMPRESSION_ZLIB 2
#define MC_CHUNK_COMPRESSION_UNCOMPRESSED 3
#define MC_CHUNK_COMPRESSION_LZ4 4
#define MC_CHUNK_COMPRESSION_CUSTOM 127

// length in bytes of the compressed NBT chunk data.
#define MC_chunk_length(chunk_data) (\
    ((unsigned int)(unsigned char)chunk_data[0]<<24) +\
    ((unsigned int)(unsigned char)chunk_data[1]<<16) +\
    ((unsigned int)(unsigned char)chunk_data[2]<<8) +\
    ((unsigned int)(unsigned char)chunk_data[3]) - 1)

// compression type used to compress the NBT chunk data.
#define MC_chunk_compression(chunk_data) (chunk_data[4])

// pointer to the compressed NBT chunk data.
#define MC_chunk_nbt_data(chunk_data) (&chunk_data[5])
