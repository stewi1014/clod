#include "map.h"

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#if __AVX512BW__
#include <immintrin.h>
#define ALIGNMENT 64

#elif __AVX2__
#include <immintrin.h>
#define ALIGNMENT 32

#elif __AVX__
#include <immintrin.h>
#define ALIGNMENT 16

#else

#define ALIGNMENT 8

#endif

void *default_malloc(const size_t alignment, const size_t size, void *) {
	return aligned_alloc(alignment, size);
}
void default_free(void *ptr, void *) { free(ptr); }
static uint32_t hash_murmur32(const void *const restrict data, const size_t data_size, const uint32_t seed) {
	const unsigned char *restrict d = data;

	uint32_t h = seed;
	uint32_t k;

	while (d < d + data_size - 4) {
		k = (uint32_t)d[0] | (uint32_t)d[1] << 8 | (uint32_t)d[2] << 16 | (uint32_t)d[3] << 24;

		k *= 0xcc9e2d51u;
		k = k << 15 | k >> (32 - 15);
		k *= 0x1b873593u;

		h ^= k;
		h = h << 13 | h >> (32 - 13);
		h = h * 5 + 0xe6546b64u;

		d += 4;
	}

	k = 0;

	switch (data_size % 4) {
		case 3: k |= (uint32_t)d[2] << 16; [[fallthrough]];
		case 2: k |= (uint32_t)d[1] << 8; [[fallthrough]];
		case 1: k |= (uint32_t)d[0];
			k *= 0xcc9e2d51u;
			k = k << 15 | k >> (32 - 15);
			k *= 0x1b873593u;

			h ^= k;
	}

	h ^= (uint32_t)data_size;

	h ^= h >> 16;
	h *= 0x85ebca6bu;
	h ^= h >> 13;
	h *= 0xc2b2ae35u;
	h ^= h >> 16;

	return h;
}

struct box {
	void *data;
	size_t size;
};

struct map {
	void *ext;
	size_t key_size;
	size_t val_size;

	void*(*malloc_f)(size_t alignment, size_t size, void *ext);
	void(*free_f)(void *ptr, void *ext);

	size_t elem_count;

	/* number of hash bits to discern bucket. */
	uint8_t bucket_bits;
	size_t bucket_count;
	union {
		void **buckets;
		void *bucket;
	};
};

#define ELEM_START_BITS 1
#define BUCKET_MAX_SIZE 256
#define ALIGN(alignment, size) (((size) + (alignment) - 1) & ~((alignment) - 1))

#define SIZEOF_BUCKET_META(elems)			ALIGN(16, (elems) * 2)
#define SIZEOF_BUCKET_KEYS(elems, key_size) ALIGN(16, (elems) * (key_size))
#define SIZEOF_BUCKET_VALS(elems, val_size) ALIGN(16, (elems) * (val_size))
#define SIZEOF_BUCKET(elems, key_size, val_size) (SIZEOF_BUCKET_META(elems) + SIZEOF_BUCKET_KEYS(elems, key_size) + SIZEOF_BUCKET_VALS(elems, val_size))

map map_init_generic(
	map *ptr,
	const size_t key_size,
	const size_t val_size,
	void *ext,
	void*(malloc_f)(size_t alignment, size_t size, void *ext),
	void(free_f)(void *ptr, void *ext)
) {
	if (!malloc_f || !free_f) {
		malloc_f = default_malloc;
		free_f = default_free;
	}

	const map m = malloc_f(alignof(struct map), sizeof(struct map), ext);

	if (!m) return nullptr;

	m->ext = ext;
	m->key_size = key_size;
	m->val_size = val_size;
	m->malloc_f = malloc_f;
	m->free_f = free_f;
	m->elem_count = 0;
	m->bucket_bits = 0;
	m->bucket_count = 0;
	m->bucket = nullptr;

	if (ptr) *ptr = m;
	return m;
}

void map_free_generic(map *ptr) {
	if (!ptr) return;
	const map m = *ptr;
	*ptr = nullptr;

	if (m->bucket_bits == 0) {
		if (m->bucket != nullptr) m->free_f(m->bucket, m->ext);
		return;
	}

	const void *last = nullptr;
	for (size_t i = 0; i < (1ULL << m->bucket_bits); i++) {
		if (m->buckets[i] != last) {
			m->free_f(m->buckets[i], m->ext);
			last = m->buckets[i];
		}
	}

	m->free_f(m->buckets, m->ext);
	m->free_f(m, m->ext);
}

void **map_ext_generic(const map m) { return &m->ext; }

void *map_set_generic(const map m, const void *key, size_t key_size) {
	if (!m || !key) return nullptr;

	void *bucket = nullptr;
	if (m->bucket_bits == 0) {
		if (m->bucket == nullptr) {
			m->bucket = m->malloc_f(ALIGNMENT, SIZEOF_BUCKET(1, m->key_size, m->val_size), m->ext);
			if (m->bucket == nullptr) return nullptr;

			m->bucket_count = 1;
		}
		bucket = m->bucket;
	} else {
		const uint32_t bucket_hash = hash_murmur32(key, m->key_size > 0 ? m->key_size : key_size, 1);
		bucket = m->buckets[bucket_hash >> (32 - m->bucket_bits)];
	}

	const uint32_t hash = hash_murmur32(key, m->key_size > 0 ? m->key_size : key_size, 0);
	const uint32_t bucket_index = hash >> (32 - m->bucket_nbits);
	const uint32_t elem_index = (hash >> 7) & (1U << m->elem_nbits) - 1;
	const uint32_t meta_hash = (hash & (1U << 7) - 1);

	void *bucket = m->buckets[bucket_index];

	#if __AVX512BW__

	__m512i group = _mm512_loadu_epi8(BUCKET_META(bucket) + (m->bucket_nbits - 1))

	#elif __AVX2__

	#else

	#endif




	if (m->bucket_count * 100 / bucket_extent > 50) {
		void **new_buckets = m->malloc_f(bucket_extent * 2 * sizeof(void *), m->ext);
		if (!new_buckets) return nullptr;

		for (size_t i = 0; i < bucket_extent; i++) {
			new_buckets[i * 2] = m->buckets[i];
			new_buckets[i * 2 + 1] = m->buckets[i];
		}

		m->free_f(m->buckets, m->ext);
		m->bucket_nbits++;
		m->buckets = new_buckets;
	}

	const uint32_t h = hash(key, m->key_size > 0 ? m->key_size : key_size, 0);
	uint32_t bucket_hash, elem_hash, meta_hash;
	bucket_hash =


	uint32_t bucket_index = h >> (32 - m->bucket_nbits);
	uint32_t elem_index = (h >> 7) & (bucket_extent - 1);
	uint32_t elem_hash = (h & 0b01111111);

	bucket *b = m->buckets[bucket_index];
	return nullptr;
}

void *map_get_generic(const map m, const void *key, size_t key_size);
void *map_del_generic(const map m, const void *key, size_t key_size);


void *map_first_generic(const map m, size_t *key_size, void **val);
void *map_next_generic(const map m, const void *key, size_t *key_size, void **val);

/*

#include <string.h>

#define _map_cmp(s1, s2, size) memcmp(s1, s2, size)
#define _map_cpy(s1, s2, size) memcpy(s1, s2, size)
static bool _map_is_zero(const unsigned char *restrict s, const size_t size) {
	for (size_t i = 0; i < size; i++)
		if (s[i] != 0) return false;
	return true;
}
static bool _map_is_ones(const unsigned char *restrict s, const size_t size) {
	for (size_t i = 0; i < size; i++)
		if (s[i] != 0xFF) return false;
	return true;
}

size_t _map_set_r(void *keys, void *vals, size_t *p_len, size_t cap, size_t k_size, size_t v_size, size_t index, void *key) {
	char cmp = _map_cmp(keys + index * k_size, key, k_size);
	if (cmp == 0) return index;

	if (cmp > 0) {
		return _map_set_r(keys, vals, p_len, cap, k_size, v_size, index << 1, key);
	} else {
		return _map_set_r(keys, vals, p_len, cap, k_size, v_size, (index << 1) + 1, key);
	}
}

// this method doesn't have enough arguments.
void *_map_set(
	void **p_keys,
	void **p_vals,
	size_t *p_len,
	size_t *p_cap,
	void *(*realloc_f)(void *, size_t),
	size_t k_size,
	size_t v_size,
	void *key,
	void *val
){
	if (*p_len == *p_cap) {
		size_t new_cap = (*p_cap << 1) - (*p_cap >> 1);
		if (new_cap == 0) new_cap = 1;

		void *new_keys = realloc_f(*p_keys, k_size * new_cap);
		void *new_vals = realloc_f(*p_vals, v_size * new_cap);

		if (new_keys) *p_keys = new_keys;
		if (new_vals) *p_vals = new_vals;

		if (!new_keys || !new_vals) {
			return nullptr;
		}

		*p_cap = new_cap;
	}

	void *set(size_t index);

	void *set(size_t index) = {
	}

	size_t index = 0;
	while (index < MAP_SIZE())


	char cmp = _map_cmp(*p_keys, key, k_size);
	while (cmp != 0) {

	}
	if (cmp == 0) {
		if (val) _map_cpy(*p_vals, val, v_size);
		return *p_vals;
	}

	size_t index = 0;
	while (index < *p_cap) {
		size_t r = _map_cmp((char*)(*))

		size_t r = memcmp((char*)(*p_keys) + index * k_size, key, k_size);
		if (r == 0) {
			if (val) memcpy((char*)(*p_vals) + index * v_size, val, v_size);

		}
	}

	*p_len++;
}




#include <assert.h>
#include <stdint.h>
#include <string.h>

struct map {
	char *keys;
	char *vals;

	void *(*realloc_f)(void *, size_t);
	size_t key_size;
	size_t val_size;

	size_t len;
	uint8_t scale;

	union elem {
		struct {
			uint32_t exists: 1;
			uint32_t hash: 31;
		};
		uint32_t val;
	} *elms;
};

#define SIZE(scale) (1 << (scale))
#define CLEAR(array, start_scale, stop_scale, elem_size) \
	memset((char*)array + SIZE(start_scale) * elem_size, 0, (SIZE(stop_scale) - SIZE(start_scale)) * elem_size)

#define NOT_EXIST SIZE_MAX

void set(const map *map, const size_t index, const uint32_t hash, const map_key *k, const map_val *v) {
	map->elms[index].exists = 1;
	map->elms[index].hash = hash;
	memcpy(map->keys + map->key_size * index, k, map->key_size);
	memcpy(map->vals + map->val_size * index, v, map->val_size);
}

void move(const map *map, const size_t dest, const size_t src, const size_t len) {
	memmove(map->elms + dest,				  map->elms + src,				   len * sizeof(*map->elms));
	memmove(map->keys + dest * map->key_size, map->keys + src * map->key_size, len * map->key_size);
	memmove(map->vals + dest * map->val_size, map->vals + src * map->val_size, len * map->val_size);
}

* never leaves the map in an invalid state.
bool resize(map *map, const uint8_t scale) {
	if (map->scale > scale) {
		if (map->len > SIZE(scale)) return false;

		size_t lo = 0, hi = SIZE(scale);
		while (lo < SIZE(scale)) {
			if (map->elms[lo].exists) {
				while (map->elms[hi].exists) hi++;
				move(map, hi, lo, 1);
				map->elms[lo].exists = 0;
			}

			lo++;
		}

		const size_t old_scale = map->scale;
		map->scale = scale;
		map->len = 0;
		for (size_t i = SIZE(scale); i < SIZE(old_scale); i++) {
			if (map->elms[i].exists)
				map_set(map, map->keys + i * map->key_size, map->vals + i * map->val_size);
			map->elms[i].exists = 0;
		}
	}

	union elem *new_data = map->realloc_f(map->elms, sizeof(union elem) * SIZE(scale));
	if (new_data) {
		map->elms = new_data;
		if (scale > map->scale) {
			for (size_t i = SIZE(map->scale); i < SIZE(scale); i++) {
				map->elms[i].exists = 0;
			}
		}
	} else {
		return false;
	}

	char *new_keys = map->realloc_f(map->keys, map->key_size * SIZE(scale));
	if (new_keys) {
		map->keys = new_keys;

	} else {
		return false;
	}

	char *new_vals = map->realloc_f(map->vals, map->val_size * SIZE(scale));
	if (new_vals || map->val_size == 0) {
		map->vals = new_vals;
	} else {
		return false;
	}

	if (scale > map->scale) {
		const size_t c = 1 << (scale - map->scale);
		for (size_t i = SIZE(map->scale); i > 0; i--) {
			move(map, i * c, i, 1);
			map->elms[i].exists = 0;
		}
	}

	map->scale = scale;
	return true;
}

* Create a new map.
map *map_new(void *(*realloc_f)(void *, size_t), const size_t k_size, const size_t v_size, size_t cap) {
	if (cap < 1) cap = 1;

	map *map = realloc_f(nullptr, sizeof(map));
	if (!map) return nullptr;

	map->realloc_f = realloc_f;
	map->key_size = k_size;
	map->val_size = v_size;
	map->len = 0;
	map->scale = 0;
	if (!map_realloc(map, cap)) {
		map_free(map);
		return nullptr;
	}

	return map;
}

* Frees resources associated with the map.
void map_free(map *m) {
	map *map = m;

	map->realloc_f(map->elms, 0);
	map->realloc_f(map->keys, 0);
	map->realloc_f(map->vals, 0);
	map->realloc_f(map, 0);
}

* Add an element to the map, returning the new value and initialising it to new if provided.
map_val *map_set(map *m, map_key *k, map_val *v) {
	const uint32_t hash = murmur_hash(k, map->key_size) &~ (1U << 31);
	if (m->len >= (SIZE(m->scale) << 1) - (SIZE(m->scale) >> 1) - (SIZE(m->scale) >> 2)) {
		if (!resize(m, m->scale + 1)) {
			return nullptr;
		}
	}

	size_t index = (size_t)hash * SIZE(m->scale) / (1U << 31);

	for (size_t i = index; i < SIZE(m->scale); i++) {
		if (m->elms[i].exists){
			if (m->elms[i].hash <= hash) {
				index = i;
			} else {
				break;
			}
		}
	}

	for (size_t i = index; i > 0; i--) {
		if (m->elms[i - 1].exists) {
			if (m->elms[i - 1].hash >= hash) {
				index = i - 1;
			} else {
				break;
			}
		}
	}

	if (m->elms[index].exists) {
		if (m->len == SIZE(m->scale)) {
			if (!map_realloc(m, m->scale + 1)) {
				return nullptr;
			}

			index = index * 2 + 1;
		} else {
			for (size_t off = 1; off < SIZE(m->scale); off++) {
				if (index > off && !m->elms[index - off].exists) {
					move(m, index - off, index - off + 1, off);
				}
				if (index + off < SIZE(m->scale) && !m->elms[index + off].exists) {
					move(m, index + 1, index, off);
				}
			}
		}
	}

	set(m, index, hash, k, v);
	m->len++;
	return m->vals + index * m->val_size;
}

* Get a value in the map, returning null if the key does not exist.
map_val *map_get(map *m, map_key *k) {
	const uint32_t hash = murmur_hash(k, m->key_size) &~ (1U << 31);
	size_t index = (size_t)hash * SIZE(m->scale) / (1U << 31);

}

* Delete an element in the map, returning the old element if it existed.
map_val *map_del(map *m, map_key *k) {

}

* Get the first element in the map. Order is undefined.
map_val *map_first(map *m);

* Get the next element in the map. Order is undefined.
map_val *map_next(map *m, map_val *v);


map *map_new(
	void *(*realloc_f)(void *, size_t),
	const size_t alignment,
	const size_t key_size,
	const size_t val_size,
	const size_t initial_capacity
) {
	map *m = realloc_f(nullptr,
		align(sizeof(map), alignment) +
		initial_capacity * (align(key_size, alignment) + align(val_size, alignment))
	);

	if (!m) return nullptr;

	m->realloc_f = realloc_f;
	m->alignment = alignment;
	m->key_size = key_size;
	m->val_size = val_size;
	m->capacity = initial_capacity;

	return m;
}

size_t find(map *m, map_key *k) {
	uint32_t h = murmur_hash(k, m->key_size, default_seed);
	size_t index = (size_t)h * m->capacity / UINT32_MAX;

	while (
		index < m->capacity - 1 &&
		(h < hdr(index)->hash || !hdr(index)->exists)
	) index++;

	while (
		index > 0 &&
		(h >= hdr(index)->hash || !hdr(index)->exists)
	) index--;

	return index;
}

* Add a key to the map, returning the new value and initialising it with a value if non-null.
map_val *map_add(map **pm, map_key *k, map_val *new) {
	map *m = *pm;
	const uint32_t h = murmur_hash(k, m->key_size, default_seed);
	const size_t want = (size_t)h * m->capacity / UINT32_MAX;
	size_t index = want;

	while (hdr(index)->exists && hdr(index)->hash > h && index > 0				) index--;
	while (hdr(index)->exists && hdr(index)->hash < h && index + 1 < m->capacity) index++;

	if (hdr(index)->exists) {

	}
}

* Get a value in the map, returning null if the key does not exist.
map_val *map_get(const map *m, map_key *k);

* Delete an element in the map, writing the old value into v if non-null and returning true if the key existed.
bool map_del(map *m, map_key *k, map_val *old);

*/
