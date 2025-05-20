/**
 * @defgroup anvil anvil.h
 * @brief Anvil methods
 * @example anvil_iter.c
 * @file anvil.h
 *
 */
#pragma once
#include <stdint.h>
#include <time.h>

/** Maximum number of coordinates the library supports. */
#define ANVIL_MAX_COORDINATES 10
/** Maximum filename size the library supports. */
#define ANVIL_MAX_FILENAME_SIZE (1<<10)

/** Number of coordinates that minecraft uses in region files. */
#define ANVIL_DEFAULT_COORDINATES 2
/** Size of sections in minecraft region files. */
#define ANVIL_DEFAULT_SECTION_SIZE 4096

typedef enum {
    ANVIL_OK            = 0,
    ANVIL_UNKNOWN       = 1,
    ANVIL_MALFORMED     = 2,
    ANVIL_ERROR_IO      = 3,
    ANVIL_INVALID_USAGE = 4,
    ANVIL_ALLOC_FAILED  = 5,
    ANVIL_LOCKED        = 6,
    ANVIL_NOT_EXIST     = 7,
    ANVIL_NO_SPACE      = 8,
} anvil_result;

/** Get the previous error. */
anvil_result anvil_error();

/** Get the message for the previous error. */
const char *anvil_error_message();

/** Clears any error value. */
void anvil_clear_error();

/** Where anvil messages are logged. stderr by default. */
extern FILE *anvil_log;

typedef struct {
    void *(*malloc)(size_t size);
    void *(*calloc)(size_t n, size_t size);
    void *(*realloc)(void *restrict ptr, size_t size);
    void (*free)(void *ptr);
} anvil_allocator;

typedef struct anvil_world anvil_world;
typedef struct anvil_dir anvil_dir;
typedef struct anvil_iter anvil_iter;

anvil_world *anvil_open(const char *path, const anvil_allocator *alloc);
void anvil_close(anvil_world *world);

anvil_dir *anvil_world_open_dir(
    const anvil_world *world,
    const char *dir,
    int64_t num_coordinates,
    size_t section_size,
    const char *region_extension,
    const char *chunk_extension
);

anvil_dir *anvil_open_dir(
    const anvil_allocator *alloc,
    const char *path,
    int64_t num_coordinates,
    size_t section_size,
    const char *region_extension,
    const char *chunk_extension
);

bool anvil_close_dir(anvil_dir *dir);

time_t anvil_mtime(anvil_dir *dir, int64_t *coordinates);

uint8_t *anvil_read(
    anvil_dir *dir,
    size_t *n_bytes_read,
    int64_t *coordinates
);

bool anvil_write(
    anvil_dir *dir,
    uint8_t *buf,
    size_t n_bytes,
    int64_t *coordinates
);

int anvil_sort_z_order(int64_t coord_count, const int64_t *, const int64_t *);
anvil_iter *anvil_iterate_dir(const anvil_dir *dir, int (*cmp)(int64_t coord_count, const int64_t *, const int64_t *));
bool anvil_iter_next(anvil_iter *iter, int64_t *coordinates);
void anvil_close_iter(anvil_iter *iter);
