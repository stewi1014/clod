/**
 * anvil.h
 * 
 * methods for dealing with the anvil world format.
 */

#pragma once
#include <stddef.h>
#include <stdint.h>

struct anvil_world;

//===============//
// World Reading //
//===============//

/**
 * same as anvil_open_ex, but reference function implementations are used.
 */
struct anvil_world *anvil_open(const char *path);

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
 * the filenames and array are passed to the free function when no longer needed.
 * 
 * open_file must return a buffer containing the file's contents,
 * and set size to the size of the buffered data, or NULL on error.
 * the idea behind char** over char*, is that an intermediate structure
 * with metadata can be returned by open_file which is then available in the close_file method.
 * 
 * close_file is called when the file is no longer needed.
 * it is always given the nonnull return value of read_file.
 * 
 * realloc_f must behave like the realloc function.
 */
struct anvil_world *anvil_open_ex(
    const char *path,
    int scandir(
        const char *restrict dirp,
        struct dirent ***restrict namelist,
        typeof(int (const struct dirent *)) *filter,
        typeof(int (const struct dirent **, const struct dirent **)) *comparator
    ),
    char **(*open_file_f)(const char *path, size_t *size),
    void (*close_file_f)(char **file),
    void *(*realloc_f)(void*, size_t)
);

/**
 * releases resources associated with the anvil world.
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
 * that facilitate this custom iteration order - but perhaps it's better to reconsider
 * whatever use case it is that depends on iterating over files in a particular order...
 */

struct anvil_region {
    char *data;
    size_t data_size;
    int64_t region_x;
    int64_t region_z;
};

struct anvil_region anvil_region_new(
    char *data,
    size_t data_size,
    int64_t region_x,
    int64_t region_z
);

struct anvil_region_iter;
struct anvil_region_iter *anvil_region_iter_new(const char *subdir, const struct anvil_world *world);
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
struct anvil_chunk_ctx *anvil_chunk_ctx_alloc(const struct anvil_world *);
void anvil_chunk_ctx_free(struct anvil_chunk_ctx *);

struct anvil_chunk {
    char *data;
    size_t data_size;
    int64_t chunk_x;
    int64_t chunk_z;
};

struct anvil_chunk anvil_chunk_decompress(
    struct anvil_chunk_ctx *,
    const struct anvil_region *,
    int64_t chunk_x,
    int64_t chunk_z
);



//=================//
// Section Reading //
//=================//

struct anvil_section {                  // contains pointers to NBT data for a specific section.
    char *end;                          // the end of the section's NBT data, and start of next section.
    char *block_state_palette;          // list NBT payload. list of blocks (compound).
    uint16_t *block_state_indices;     // 16x16x16 array of block state indices. inaccurate if block state palette size is <= 1
    char *biome_palette;                // list NBT payload. list of biomes (string).
    uint16_t *biome_indices;           // 4x4x4 array of biome indices. inaccurate if biome palette size is <= 1
    char *block_light;                  // 2048 byte array
    char *sky_light;                    // 2048 byte array
};

struct anvil_sections {
    struct anvil_section *section;      // array of sections.
    int64_t len;                        // the number of sections in the array.
    int64_t cap;                        // the size (in sections) of the allocated array.

    int64_t x;
    int64_t z;
    int64_t min_y;                      // the lowest section y value. can be added to index to get section Y.

    char *status;                       // Chunk Status NBT tag.
    char *start;                        // the start of the first section's data.
    void *(*realloc)(void*, size_t);    // the method used to allocate section data.
};

#define ANVIL_SECTIONS_CLEAR (struct anvil_sections){NULL, 0, 0}

/**
 * parses the sections in the chunk into an array.
 * 
 * the array is sorted in descending y order.
 * 
 * sections must be ANVIL_SECTIONS_CLEAR, or the result of a previous call to anvil_arse_sections.
 * the caller is responsible for freeing the array when done.
 * 
 * it returns nonzero on failure.
 */
int anvil_parse_sections_ex(
    struct anvil_sections *sections,
    struct anvil_chunk,
    void *(*realloc_f)(void*, size_t)
);

/**
 * parses the sections in the chunk into an array.
 * 
 * the array is sorted in descending y order.
 * 
 * sections must be ANVIL_SECTIONS_CLEAR, or the result of a previous call to anvil_arse_sections.
 * the caller is responsible for freeing the array when done.
 * 
 * it returns nonzero on failure.
 */
static inline
int anvil_parse_sections(
    struct anvil_sections *sections,
    const struct anvil_chunk chunk
) {
    return anvil_parse_sections_ex(sections, chunk, NULL);
}

/** releases resources associated with the sections. */
void anvil_sections_free(
    struct anvil_sections *sections
);
