#pragma once
#ifndef ANVIL_H
#error Do not include the internal header directly
#endif

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#ifdef NDEBUG
    #define anvil_breakpoint()
#else
    #ifdef _MSC_VER
        #define anvil_breakpoint() __debugbreak()
    #else
        #include <signal.h>
        #define anvil_breakpoint() raise(SIGTRAP)
    #endif
#endif

#define mutex_t pthread_mutex_t
#define mutex_init(m) pthread_mutex_init(m, nullptr)
#define mutex_destroy(m) pthread_mutex_destroy(m)
#define mutex_lock(m) pthread_mutex_lock(m)
#define mutex_unlock(m) pthread_mutex_unlock(m)

//=======//
// Types //
//=======//

typedef struct region_file region_file;
typedef struct chunk_file chunk_file;
typedef struct buffer buffer;

struct anvil {
    const char *path;
    mutex_t mutex;

    region_file *region_files;

    size_t buffers_len;
    size_t buffers_cap;
    char *buffers;

    anvil_opts opts;

    #ifdef CLOD_POSIX
    int dir_fd;
    #else
    #error not implemented
    #endif

    char __ext[];
};

#define CHUNK_COUNT(opts)   (powl(opts.region_extent, opts.coord_count))
#define TABLE_SIZE(opts)    (CHUNK_COUNT(opts) * 8)

#define CHUNK_TABLE_OFF(opts)       (TABLE_SIZE(opts) * 0)
#define MTIME_TABLE_OFF(opts)       (TABLE_SIZE(opts) * 1)
    #define HEADER_SIZE(opts)       (TABLE_SIZE(opts) * 2)

#define CHECKSUM_TABLE_OFF(opts)    (TABLE_SIZE(opts) * 2)
#define SH_CHECKSUM_TABLE_OFF(opts) (TABLE_SIZE(opts) * 3)
#define CHECKSUM_OFF(opts)          (TABLE_SIZE(opts) * 4)
    #define SH_HEADER_SIZE(opts)    (TABLE_SIZE(opts) * 4 + 4)

#define CHUNK_SEPARATE(data)    (data[4] & 0b10000000 > 0)
#define CHUNK_COMPRESSION(data) (data[4] & 0b01111111)

struct region_file {
    size_t ref_count;
    int64_t *region_pos;

    chunk_file *chunk_files;

    size_t data_size;
    uint8_t *data;

    uint8_t *sh;

    #ifdef CLOD_POSIX
    int fd;
    #else
    #error not implemented
    #endif
};

struct chunk_file {
    size_t ref_count;
    int64_t *chunk_pos;

    size_t data_size;
    uint8_t *data;

    int8_t compression;

    #ifdef CLOD_POSIX
    int fd;
    #else
    #error not implemented
    #endif
};

#define sizeof_buffer(coord_count) (sizeof(buffer) + sizeof(int64_t) * coord_count)
struct buffer {
    uint8_t *ptr;
    int64_t ref_count;
    region_file *rfile;
    chunk_file *cfile;
    int64_t coords[];
};

//==================//
// Internal Methods //
//==================//

region_file *region_file_open(anvil *a, region_file *old_file, const int64_t *region_pos);

uint8_t *region_file_read(anvil *a, region_file *f, const int64_t *chunk_pos, size_t *size);
uint8_t *region_file_write(anvil *a, region_file *f, const int64_t *chunk_pos, size_t size);
anvil_result region_file_close(anvil *a, region_file *f);

chunk_file *chunk_file_open(anvil *a, chunk_file *old_file, const int64_t *chunk_pos);
uint8_t *chunk_file_read(anvil *a, chunk_file *f, size_t *size);
uint8_t *chunk_file_write(anvil *a, chunk_file *f, size_t size);
anvil_result chunk_file_close(anvil *a, chunk_file *f);

int64_t anvil_parse_filename(
    const char *name,
    const char **extension_ptr,
    int64_t coord_count,
    int64_t *coords
);

const char *anvil_create_filename(
    const char *prefix,
    const char *extension,
    int64_t coord_count,
    const int64_t *coords
);

static uint32_t read_be32(const uint8_t *d) {
    return d[0] << (3 * 8) | d[1] << (2 * 8) | d[2] << (1 * 8) | d[3] << (0 * 8);
}

static void write_be32(uint8_t *d, const uint32_t v) {
    d[0] = v >> (3 * 8); d[1] = v >> (2 * 8); d[2] = v >> (1 * 8); d[3] = v >> (0 * 8);
}

#define chunk_filename(opts, coords)        anvil_create_filename("c",      opts.chunk_extension, opts.coord_count, coords)
#define chunk_sh_filename(opts, coords)     anvil_create_filename("sh_c",   opts.chunk_extension, opts.coord_count, coords)

#define filename(opts, coords)        anvil_create_filename("r",      opts.region_extension, opts.coord_count, coords)
#define sh_filename(opts, coords)     anvil_create_filename("sh_r",   opts.region_extension, opts.coord_count, coords)
#define new_filename(opts, coords)    anvil_create_filename("new_r",  opts.region_extension, opts.coord_count, coords)

//================//
// Error Handling //
//================//

#define MAX_ERROR_MSG 1024

_Thread_local extern anvil_result anvil_result_value;
_Thread_local extern char error_message[MAX_ERROR_MSG];
extern void (*error_log)(const char *msg);

#define set_errno(format, ...) set_error(errno_mapping(errno), strerror(errno), format, ##__VA_ARGS__)
#define set_result(res, format, ...) set_error(res, anvil_result_strerror(errno), format, ##__VA_ARGS__)
#define anvil_assert(expr) ((expr) ? ANVIL_OK :(\
    set_error(ANVIL_INVALID_USAGE, "Assertion failed", "%s()[%s:%d] %s", __func__, __FILE__, __LINE__, #expr),\
    ANVIL_INVALID_USAGE\
))

#define preserve_error(code) {\
    const anvil_result anvil_result_value_original = anvil_result_value;\
    char anvil_error_message_original[MAX_ERROR_MSG];\
    strncpy(anvil_error_message_original, anvil_error_message, MAX_ERROR_MSG);\
    code;\
    anvil_result_value = anvil_result_value_original;\
    strncpy(anvil_error_message, anvil_error_message_original, MAX_ERROR_MSG);\
}

const char *anvil_result_strerror(anvil_result res);
anvil_result errno_mapping(error_t err);
void anvil_set_error(anvil_result res, const char *err_str, const char *format, ...);
