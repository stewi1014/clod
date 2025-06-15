#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "map.h"

static uint32_t pext_u32(uint32_t x, uint32_t y);

#ifdef CLOD_HAS_IMMINTRIN_PEXT_U32

#include <immintrin.h>
static uint32_t pext_u32(uint32_t x, uint32_t y) {
    return _pext_u32(x, y);
}

#else

static uint32_t pext_u32(const uint32_t x, uint32_t y) {
    uint32_t res = 0, off = 0;
    do {
        const uint32_t lsb = y & -y;
        y &= ~lsb;
        if (x & lsb) {
            res |= 1 << off;
        }
        off += 1;
    } while (y);
    return res;
}

#endif

typedef struct map_node map_node;

struct map_node {
    uint32_t hash;
    uint32_t hash_mask;
    union {

    } elements[];
};

struct map_gen{
    map_node node;
    map_realloc_fn *realloc_fn;
};

uint32_t hash(const uint8_t *data, const size_t size, uint32_t seed);

map_gen *map_new_gen(map_realloc_fn *realloc_fn);
map_val map_add_gen(map_gen *map, void *key, size_t key_size, void *val, size_t val_size);

//===============//
// Hash Function //
//===============//

uint32_t hash(const uint8_t *data, const size_t size, const uint32_t seed) {
    // Based on Murmur Hash reference implementation.
    // https://github.com/aappleby/smhasher/blob/0ff96f7835817a27d0487325b6c16033e2992eb5/src/MurmurHash3.cpp#L94

    const map(uint32_t) map = map_new(realloc, 0);

    map.map->realloc_fn(nullptr, 1);

    const auto key = (map_key){"asdf", sizeof("asdf")};
    const auto val = (map_val){"test2", sizeof("test2")};


    map_add_gen(map.map, "asdf", sizeof("asdf"), "test2", sizeof("test2"));

};
