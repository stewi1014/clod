#ifndef MAP_H
#define MAP_H

#include <stddef.h>
#include <stdint.h>

typedef struct map *map;
#define map(key_type, val_type) union { \
	struct {\
		map map;\
		void *tmp;\
	};\
	key_type (*_key_type);\
	val_type (*_val_type);\
}

#define _map_type(_map) typeof(_map)
#define _map_pkey_type(_map) typeof((_map)._key_type)
#define _map_pval_type(_map) typeof((_map)._val_type)
#define _map_arg1(V, ...) V

#define MAP_BOXED 0

#define map_init(map_, key_size, val_size, ext, malloc_f, free_f) ((_map_type(map_)*)map_init_generic(&(map_).map, key_size, val_size, ext, malloc_f, free_f))
map map_init_generic(map *ptr, size_t key_size, size_t val_size, void *ext, void*(malloc_f)(size_t alignment, size_t size, void *ext), void(free_f)(void *ptr, void *ext));

#define map_free(map_) map_free_generic(&(map_).map)
void map_free_generic(map *ptr);

#define map_ext(map_) (*map_ext_generic((map_).map))
void **map_ext_generic(map m);

#define map_set(map_, key, ...) ((_map_pval_type(map_))map_set_generic((map_).map, key, _map_arg1(__VA_ARGS__ __VA_OPT__(,) 0)))
void *map_set_generic(map m, const void *key, size_t key_size);

#define map_get(map_, key, ...) ((_map_pval_type(map_))map_get_generic((map_).map, key, _map_arg1(__VA_ARGS__ __VA_OPT__(,) 0)))
void *map_get_generic(map m, const void *key, size_t key_size);

#define map_del(map_, key, ...) ((_map_pval_type(map_))map_del_generic((map_).map, key, _map_arg1(__VA_ARGS__ __VA_OPT__(,) 0)))
void *map_del_generic(map m, const void *key, size_t key_size);

#define map_foreach(map_, key, val, ...) \
for (\
	(\
		key = (_map_pkey_type(map_))map_first_generic((map_).map, _map_arg1(__VA_OPT__(&)__VA_ARGS__ __VA_OPT__(,) (void*)0), &(map_).tmp),\
		val = (_map_pval_type(map_))(map_).tmp\
	); key; (\
		key = (_map_pkey_type(map_))map_next_generic((map_).map, key, _map_arg1(__VA_OPT__(&)__VA_ARGS__ __VA_OPT__(,) (void*)0), &(map_).tmp),\
		val = (_map_pval_type(map_))(map_).tmp\
	)\
)

void *map_first_generic(map m, size_t *key_size, void **val);
void *map_next_generic(map m, const void *key, size_t *key_size, void **val);

#endif //MAP_H

/*

#include <stddef.h>

#define MAP_SIZE(depth) ((1 << depth) - 1)

** Map definition.

#define map(key_type, val_type) \
struct {\
	key_type *keys;\
	val_type *vals;\
	size_t len;\
	size_t cap;\
	void *(*realloc)(void *, size_t);\
	size_t k_size;\
	size_t v_size;\
	void *_ext;\
}

** Initialise a map.
#define map_init(map, realloc_f, key_size, val_size, initial_cap) (\
	((map).realloc = realloc_f),\
	((map).k_size = key_size),\
	((map).v_size = val_size),\
	((map).len = 0),\
	((map).cap = initial_cap),\
	((map).keys = (map).cap > 0 ? (map).realloc(nullptr, (map).k_size * (map).cap) : nullptr),\
	((map).vals = (map).cap > 0 ? (map).realloc(nullptr, (map).v_size * (map).cap) : nullptr),\
	((map).cap = (map).keys && (map).vals ? (map).cap : 0)\
)

** Free resources associated with the map.
#define map_free(map) \
	((map).realloc((map).keys, 0), (map).realloc((map).vals, 0))

** Add or modify an entry in the map.
#define map_set(map, key, ...) _map_set_switch(map, key, __VA_ARGS__, _map_set3, _map_set2)(map, key, __VA_ARGS__)
#define _map_set_switch(_1, _2, _3, FN, ...) FN
#define _map_set2(map, key) ((typeof((map).vals))_map_set(&(map).keys, &(map).vals, &(map).len, &(map).cap, (map).realloc, (map).k_size, (map).v_size, &(key), nullptr))
#define _map_set3(map, key, val) ((typeof((map).vals))_map_set(&(map).keys, &(map).vals, &(map).len, &(map).cap, (map).realloc, (map).k_size, (map).v_size, &(key), &(val)))
void *_map_set(void **p_keys, void **p_vals, size_t *p_len, size_t *p_cap, void *(*realloc_f)(void *, size_t), size_t k_size, size_t v_size, void *key, void *val);

#define map_get(map, key, ...) _map_get_switch(map, key, __VA_ARGS__, _map_get3, _map_get2)(map, key, __VA_ARGS__)
#define _map_get_switch(_1, _2, _3, FN, ...) FN
#define _map_get_2(map, key) ((typeof((map).vals))_map_get((map).keys, (map).vals, (map).len, (map).k_size, (map).v_size, key))
#define _map_get_3(map, key, val) ((typeof((map).vals))((map)._ext = _map_get((map).keys, (map).vals, (map).len, (map).k_size, (map).v_size, key), (map)._ext ? (map)._ext : &(val)))
void *_map_get(void *keys, void *vals, size_t len, size_t k_size, size_t v_size, void *key);

#define map_del(map, key)

** Add or update an element to the map, returning the new value and setting it to new if provided. Value is zeroed if new is null.
map_val *map_set(map m, map_key *k, map_val *v);

** Get a value in the map, returning null if the key does not exist.
map_val *map_get(map m, map_key *k);

** Delete an element in the map, returning the old element if it existed. *
map_val *map_del(map m, map_key *k);

** Get the first element in the map. Order is undefined. *
map_val *map_first(map m);

** Get the next element in the map. Order is undefined. *
map_val *map_next(map m, map_val *v);

*/
