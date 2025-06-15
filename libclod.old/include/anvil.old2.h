/**
 * @defgroup anvil anvil.h
 * @brief Anvil methods
 * @example anvil_iter.c
 * @file anvil.h
 *
 */
#pragma once
#include <stdint.h>
#include <stdio.h>
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
anvil_result anvil_get_error();

/** Get the message for the previous error. */
const char *anvil_strerror();

/** Clears any error value. separate */
void anvil_clear_error();

/** Where anvil messages are logged. stderr by default. */
extern FILE *anvil_log;

typedef struct {
    void *(*malloc)(size_t size);
    void *(*calloc)(size_t n, size_t size);
    void *(*realloc)(void *restrict ptr, size_t size);
    void (*free)(void *ptr);
} anvil_allocator;

typedef struct anvil_dir anvil_dir;
typedef struct anvil_iter anvil_iter;

typedef struct {
    int64_t count;
    int64_t extent;
} anvil_coord_layout;

typedef struct {
    anvil_coord_layout layout;
    size_t section_size;

    const char *region_extension;
    const char *chunk_extension;

    anvil_allocator *alloc;
} anvil_opts;

#define ANVIL_DEFAULT_OPTS (struct anvil_opts){\
    (anvil_coord_layout){2, 32},\
    4096, "mcr", "mcc", nullptr, false\
}

anvil_dir *anvil_open(
    const char *path,
    struct anvil_opts *opts_ptr
);

void anvil_close(anvil_dir *dir);

uint32_t anvil_mtime(anvil_dir *dir, int64_t *chunk_coords);

uint8_t *anvil_read(
    anvil_dir *dir,
    const int64_t *chunk_coords,
    size_t *chunk_size
);

anvil_result anvil_write(
    anvil_dir *dir,
    const int64_t *chunk_coords,
    const uint8_t *chunk_data,
    size_t chunk_size
);

uint8_t *anvil_read(
    anvil_dir *dir,
    size_t *n_bytes_read,
    const int64_t *chunk_coords
);

bool anvil_write(
    anvil_dir *dir,
    const uint8_t *buf,
    size_t n_bytes,
    const int64_t *chunk_coords
);

int anvil_sort_z_order(int64_t coord_count, const int64_t *chunk_coords1, const int64_t *chunk_coords2);
anvil_iter *anvil_iterate_dir(const anvil_dir *dir, int (*cmp)(int64_t coord_count, const int64_t *chunk_coords1, const int64_t *chunk_coords2));
bool anvil_iter_next(anvil_iter *iter, int64_t *chunk_coords);
void anvil_close_iter(anvil_iter *iter);
