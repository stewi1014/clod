#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <dirent.h>

#if defined(__unix__) || defined(__APPLE__)
#define UNISTD_SESSION_LOCK
#include <unistd.h>
#endif

#include <libdeflate.h>
#include <lz4.h>

#include "file.h"
#include "anvil.h"

#define MAX_PATH_LEN 4096 // I hate this
#define BUFFER_START 100 * 1024
#define BUFFER_MAX 4 * 1024 * 1024


//=======//
// World //
//=======//

struct anvil_world {
    char *path;
    FILE *session_lock_file;
    char **(*open_file)(char *path, size_t *size);
    void (*close_file)(char **file);
    void *(*malloc)(size_t);
    void *(*realloc)(void*, size_t);
    void (*free)(void*);
};

struct anvil_world *anvil_open(char *path) {
    return anvil_open_ex(path, NULL, NULL, NULL, NULL, NULL);
}

struct anvil_world *anvil_open_ex(
    char *path,
    char **(*open_file_f)(char *path, size_t *size),
    void (*close_file_f)(char **file),
    void *(*malloc_f)(size_t),
    void *(*realloc_f)(void*, size_t),
    void (*free_f)(void*)
) {
    if (open_file_f == NULL) open_file_f = 📤;
    if (close_file_f == NULL) close_file_f = 📥;
    if (malloc_f == NULL) malloc_f = malloc;
    if (realloc_f == NULL) realloc_f = realloc;
    if (free_f == NULL) free_f = free;

    struct anvil_world *world = malloc_f(sizeof(struct anvil_world));
    if (world == NULL) {
        return NULL;
    }

    size_t path_size = strlen(path);
    if (path_size >= strlen("level.dat") && !strcmp(path + path_size - strlen("level.dat"), "level.dat")) {
        path_size -= strlen("level.dat");
    }
    char *world_path = malloc_f(MAX_PATH_LEN);
    if (world_path == NULL) {
        free_f(world);
        return NULL;
    }
    memcpy(world_path, path, path_size);
    world_path[path_size] = '\x0';

    world_path[path_size] = PATH_SEPERATOR;
    strcpy(world_path + path_size + 1, "session.lock");
    FILE *session_lock = fopen(world_path, "w");
    world_path[path_size] = '\x0';
    if (session_lock == NULL) {
        free_f(world_path);
        free_f(world);
        return NULL;
    }

    if (fputs("☃", session_lock) < 0) {
        fclose(session_lock);
        free_f(world_path);
        free_f(world);
        return NULL;
    }
    if (fflush(session_lock)) {
        fclose(session_lock);
        free_f(world_path);
        free_f(world);
        return NULL;
    }

    #ifdef UNISTD_SESSION_LOCK
        if (lockf(fileno(session_lock), F_LOCK, -ftell(session_lock))) {
            // hmmmm
            // I have half a mind to ignore this - but I won't.
            fclose(session_lock);
            free_f(world_path);
            free_f(world);
            return NULL;
        }
    #endif

    world->path = world_path;
    world->open_file = open_file_f;
    world->close_file = close_file_f;
    world->malloc = malloc_f;
    world->realloc = realloc_f;
    world->free = free_f;
    return world;
}

void anvil_close(struct anvil_world *world) {
    world->free(world->path);
    world->free(world);
}



//========//
// Region //
//========//

struct anvil_region anvil_region_new(
    char *data,
    size_t data_size,
    int region_x,
    int region_z
) {
    assert(data != NULL);
    assert(data_size >= 8192);

    struct anvil_region region;
    region.data = data;
    region.data_size = data_size;
    region.region_x = region_x;
    region.region_z = region_z;
    return region;
}

struct anvil_region_iter {
    char *path;
    DIR *dir;
    char **previous_file;
    char **(*open_file)(char *path, size_t *size);
    void (*close_file)(char **file);
    void (*free)(void*);
};

struct anvil_region_iter *anvil_region_iter_new(char *subdir, struct anvil_world *world) {
    if (world == NULL) return NULL;

    struct anvil_region_iter *iter = world->malloc(sizeof(struct anvil_region_iter));
    if (iter == NULL) {
        return NULL;
    }

    char *path = world->malloc(MAX_PATH_LEN);
    strcpy(path, world->path);

    if (subdir != NULL) {
        size_t len = strlen(path);
        path[len] = PATH_SEPERATOR;
        strcpy(path + len + 1, subdir);
    }

    DIR *dir = opendir(path);

    if (dir == NULL) {
        world->free(iter);
        return NULL;
    }

    iter->path = path;
    iter->dir = dir;
    iter->previous_file = NULL;
    iter->open_file = world->open_file;
    iter->close_file = world->close_file;
    iter->free = world->free;
    return iter;
}

int anvil_region_iter_next(
    struct anvil_region *region,
    struct anvil_region_iter *iter
) {
    assert(region != NULL);
    if (iter == NULL) {
        return -1;
    }

    if (iter->previous_file != NULL) {
        iter->close_file(iter->previous_file);
        iter->previous_file = NULL;
    }

    int path_size = strlen(iter->path);
    struct dirent *ent;
    int scan_count;

next_region:
    ent = readdir(iter->dir);
    if (ent == NULL) {
        return 1;
    }

    scan_count = sscanf(ent->d_name, "r.%d.%d.mca", &region->region_x, &region->region_z);
    if (scan_count != 2) {
        goto next_region;
    }

    iter->path[path_size] = PATH_SEPERATOR;
    strcpy(iter->path + path_size + 1, ent->d_name);

    char **file = iter->open_file(iter->path, &region->data_size);
    iter->path[path_size] = '\x0';
    if (file == NULL) {
        return -1;
    }

    if (region->data_size == 0) {
        iter->close_file(file);
        goto next_region;
    }

    iter->previous_file = file;
    region->data = *file;
    if (region->data == NULL) {
        return -1;
    }

    return 0;
}

void anvil_region_iter_free(struct anvil_region_iter *iter) {
    if (iter == NULL) {
        return;
    }

    if (iter->previous_file != NULL) {
        iter->close_file(iter->previous_file);
    }

    closedir(iter->dir);
    iter->free(iter->path);
    iter->free(iter);
}


//=======//
// Chunk //
//=======//

struct anvil_chunk_ctx {
    char *buffer;
    size_t buffer_cap;

    struct libdeflate_decompressor *libdeflate_decompressor;

    void *(*malloc)(size_t);
    void *(*realloc)(void*, size_t);
    void (*free)(void*);
};

struct anvil_chunk_ctx *anvil_chunk_ctx_alloc(struct anvil_world *world) {
    struct anvil_chunk_ctx *ctx;
    if (world == NULL) {
        ctx = malloc(sizeof(struct anvil_chunk_ctx));
        if (ctx == NULL) return NULL;
        ctx->malloc = malloc;
        ctx->realloc = realloc;
        ctx->free = free;
    } else {
        ctx = world->malloc(sizeof(struct anvil_chunk_ctx));
        if (ctx == NULL) return NULL;
        ctx->malloc = world->malloc;
        ctx->realloc = world->realloc;
        ctx->free = world->free;
    }

    ctx->libdeflate_decompressor = NULL;
    ctx->buffer_cap = BUFFER_START;
    ctx->buffer = ctx->malloc(ctx->buffer_cap);
    if (ctx->buffer == NULL) {
        ctx->free(ctx);
        return NULL;
    }

    return ctx;
}

void anvil_chunk_ctx_free(struct anvil_chunk_ctx *ctx) {
    if (ctx->libdeflate_decompressor != NULL) {
        libdeflate_free_decompressor(ctx->libdeflate_decompressor);
    }

    if (ctx->buffer != NULL) {
        ctx->free(ctx->buffer);
    }

    ctx->free(ctx);
}

static struct libdeflate_decompressor *chunk_ctx_libdeflate_decompressor(struct anvil_chunk_ctx *ctx) {
    if (ctx->libdeflate_decompressor == NULL) {
        struct libdeflate_options options;
        memset(&options, 0, sizeof(options));
        options.sizeof_options = sizeof(options);

        options.malloc_func = ctx->malloc;
        options.free_func = ctx->free;

        ctx->libdeflate_decompressor = libdeflate_alloc_decompressor_ex(&options);
    }
    return ctx->libdeflate_decompressor;
}

#define REGION_COMPRESSION_GZIP 1
#define REGION_COMPRESSION_ZLIB 2
#define REGION_COMPRESSION_UNCOMPRESSED 3
#define REGION_COMPRESSION_LZ4 4
#define REGION_COMPRESSION_CUSTOM 127

struct anvil_chunk anvil_chunk_decompress(
    struct anvil_chunk_ctx *ctx,
    struct anvil_region *region,
    int chunk_x,
    int chunk_z
) {
    chunk_x &= 31;
    chunk_z &= 31;

    struct anvil_chunk chunk;
    chunk.data = NULL;
    chunk.data_size = 0;
    chunk.chunk_x = chunk_x;
    chunk.chunk_z = chunk_z;
    if (region->data_size == 0) {
        chunk.data = "";
        return chunk;
    } else if (region->data_size < 8192) {
        return chunk;
    }

    int chunk_index = chunk_x + chunk_z * 32;
    int offset = 
        ((int)(unsigned char)region->data[0 + 4 * chunk_index] << 16) +
        ((int)(unsigned char)region->data[1 + 4 * chunk_index] << 8) +
        ((int)(unsigned char)region->data[2 + 4 * chunk_index]);
    unsigned char sectors =
        region->data[3 + 4 * chunk_index];

    if (offset < 2 || sectors == 0) {
        chunk.data = "";
        return chunk;
    }

    char *chunk_data = region->data + 4096 * offset;

    int compressed_size =
        ((int)(unsigned char)chunk_data[0] << 24) +
        ((int)(unsigned char)chunk_data[1] << 16) +
        ((int)(unsigned char)chunk_data[2] << 8)  +
        ((int)(unsigned char)chunk_data[3]) - 1;

    switch (chunk_data[4]) {
    case REGION_COMPRESSION_GZIP:
    case REGION_COMPRESSION_ZLIB: {
        struct libdeflate_decompressor *decompressor = chunk_ctx_libdeflate_decompressor(ctx);
        size_t decompressed_size;
        enum libdeflate_result result;

    try_deflate_again:
        if (chunk_data[4] == REGION_COMPRESSION_GZIP) {
            result = libdeflate_gzip_decompress(
                decompressor, 
                &chunk_data[5], 
                compressed_size,
                ctx->buffer,
                ctx->buffer_cap,
                &decompressed_size
            );
        } else if (chunk_data[4] == REGION_COMPRESSION_ZLIB) {
            result = libdeflate_zlib_decompress(
                decompressor, 
                &chunk_data[5], 
                compressed_size,
                ctx->buffer,
                ctx->buffer_cap,
                &decompressed_size
            );
        } else {
            assert(false);
        }

        if (result == LIBDEFLATE_INSUFFICIENT_SPACE){
            size_t new_capacity = ctx->buffer_cap * 2;
            char *new_buffer = realloc(ctx->buffer, new_capacity);
    
            if (new_buffer != NULL) {
                ctx->buffer = new_buffer;
                ctx->buffer_cap = new_capacity;
                goto try_deflate_again;
            }
        }

        if (result == LIBDEFLATE_SUCCESS || result == LIBDEFLATE_SHORT_OUTPUT) {
            chunk.data = ctx->buffer;
            chunk.data_size = decompressed_size;
        }

        return chunk;
    };
    case REGION_COMPRESSION_UNCOMPRESSED: {
        chunk.data = &chunk_data[5];
        chunk.data_size = compressed_size;
        return chunk;
    }
    case REGION_COMPRESSION_LZ4: {
        // minecraft's "LZ4" format is a custom format that does not conform to the lz4 frame format.
        // AFAICT it's unlikely to ever be supported here, unless someone makes a C library for it.
    };
    case REGION_COMPRESSION_CUSTOM: {
        return chunk;
    }
    default: return chunk;
    }
}
