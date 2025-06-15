#pragma once
#include <stddef.h>

typedef void*(map_realloc_fn)(void*, size_t);

typedef struct map_key {
    void *key;
    size_t key_size;
} map_key;

typedef struct map_val {
    void *val;
    size_t val_size;
} map_val;

typedef struct map_gen map_gen;
#define map(type)\
struct {\
    map_gen *const map;\
    const type zero;\
}

#define MAP_NULL           (map_val){0, ((void*)0)}
#define MAP_REALLOC_FAILED (map_val){1, ((void*)0)}

map_gen *map_new_gen(map_realloc_fn realloc_fn);
#define map_new(realloc_fn, zero) { map_new_gen(realloc_fn), zero }

map_val map_add_gen(map_gen *map, void *key, size_t key_size, void *val, size_t val_size);
#define map_add(map, key, val) (\
    ((map).zero == (val)) ? true :\
    map_add_gen((map).map, &key, sizeof(key), &val, sizeof(val))\
)

map_val map_get_gen(const map_gen *map, map_key key);
#define map_get(map, key) \
    ((typeof((map).zero)*)map_get_gen((map).map, key, sizeof(key)))
