/**
 * anvil.h
 * 
 * methods for dealing with the anvil world format.
 */

#pragma once
#include <stddef.h>

struct anvil_world;

//===============//
// World Reading //
//===============//

/**
 * same as anvil_open_ex, but reference function implementations are used.
 */
struct anvil_world *anvil_open(char *path);

/**
 * opens an anvil-format minecraft world.
 * 
 * path is the path to the world folder, or level.dat file.
 * 
 * when opening a world, a copy of the path is made,
 * and if the path ends in "level.dat" it is trimmed.
 * 
 * any of the following function arguments may be null,
 * and their default implementation will be used instead.
 * 
 * read_dir must return a null-terminated array of null-terminated
 * filenames for files in the given directory, or NULL on error.
 * the filnames and array are passed to the free function when no longer needed.
 * 
 * open_file must return a buffer containing the file's contents,
 * and set size to the size of the buffered data, or NULL on error.
 * the idea behind char** over char*, is that an intermediate structure
 * with metadata can be returned by open_file which is then available in the close_file method.
 * 
 * close_file is called when the file is no longer needed.
 * it is always given the nonnull return value of read_file.
 * 
 * malloc_f must behave like the malloc function.
 * 
 * realloc_f must behave like the realloc function.
 * 
 * free_f *surprise* must behave like the free function.
 */
struct anvil_world *anvil_open_ex(
    char *path,
    char **(*open_file)(char *path, size_t *size),
    void (*close_file)(char **file),
    void *(*malloc_f)(size_t),
    void *(*realloc_f)(void*, size_t),
    void (*free_f)(void*)
);

/**
 * releases resources accociated with the anvil world.
 */
void anvil_close(struct anvil_world *);



//================//
// Region Reading //
//================//

/**
 * The idea behind region reading is to give the user guidance on what region files to read.
 * Implementing a getter API here doesn't work as the user doesn't know what regions actually exist in the world.
 * You can't much iterate over every possible region file that might exist.
 * 
 * So an iterator that allows the user to visit every region file is made available.
 * If a particular order of visitation is required then some new iter method/s can be added
 * that facilitate this custom iteration order - but perhapse it's better to reconsider
 * whatever use case it is that depends on iterating over files in a particular order...
 */

struct anvil_region {
    char *data;
    size_t data_size;
    int region_x;
    int region_z;
};

struct anvil_region anvil_region_new(
    char *data,
    size_t data_size,
    int region_x,
    int region_z
);

struct anvil_region_iter;
struct anvil_region_iter *anvil_region_iter_new(char *subdir, struct anvil_world *world);
void anvil_region_iter_free(struct anvil_region_iter *);
int anvil_region_iter_next(
    struct anvil_region *,
    struct anvil_region_iter *
);



//===============//
// Chunk Reading //
//===============//

/**
 * The general idea behind chunk reading is that anvil_chunk_decompress uses the persisting
 * resources (large allocated buffers & decompressors) in anvil_chunk_ctx to provide
 * a transient view into chunk data that is only valid until the next call with the same context (resources are then reused).
 * 
 * This means that only one chunk can be loaded at a time, except it doesn't.
 * If you need to load 4 chunks at once you can create 4 anvil_chunk_ctx's and use them without any special considerations.
 */

struct anvil_chunk_ctx;
struct anvil_chunk_ctx *anvil_chunk_ctx_alloc(struct anvil_world *);
void anvil_chunk_ctx_free(struct anvil_chunk_ctx *);

struct anvil_chunk {
    char *data;
    size_t data_size;
    int chunk_x;
    int chunk_z;
};

struct anvil_chunk anvil_chunk_decompress(
    struct anvil_chunk_ctx *,
    struct anvil_region *,
    int chunk_x,
    int chunk_z
);



//=================//
// Section Reading //
//=================//

/**
 * We somewhat return to the region file approach here - and for simmilar reasons too.
 * 
 * The number of sections (16x16x16) in a chunk is not set in stone.
 * Sure, vanilla minecraft only has 16, but mods oftenn increase this limit - and the game facilitates this.
 * 
 * Sections are often very small, but are sometimes very large.
 * This makes visiting the NBT data and keeping pointers to the relevent data for each section very inefficient,
 * with the intermediate pointer structure being almost as large as the section data itself.
 * 
 * And parsing all section data into an intermediate format is most certainly out of the question,
 * as it would be highly pressumptious of me to do a bunch of computing to create a data structure
 * the user didn't ask for here.
 * 
 * So, we have an iterator. The user is taken along for a ride through all the sections in the chunk,
 * and what they choose to parse, copy or ignore is up to them.
 * 
 * Notably, this iterates in the order the sections appear in the list - not based on the Y value.
 * It seems as though the sections always appear in order.
 * Figuring this out who to trust is left as an excercise for the user.
 */

struct anvil_section_iter {
    char *next_section;         // pointer to the byte after this section.
    char *end;                  // end of nbt data.
    int section_count;          // the number of sectiosn in the section list.
    int current_index;          // the index of the current section.

    int section_y;              // Y coordinate of the section.
    char *block_state_palette;  // list NBT tag. list of blocks (compound).
    char *block_state_array;    // long array NBT payload. packed block state palette indicies.
    char *biome_palette;        // list NBT tag. list of biomes (string).
    char *biome_array;          // long array NBT payload. packed biome palette indicies.
    char *block_light;          // byte array NBT payload.
    char *sky_light;            // byte array NBT payload.
};

int anvil_section_iter_init(
    struct anvil_section_iter *,
    struct anvil_chunk
);

int anvil_section_iter_next(struct anvil_section_iter *);
