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
