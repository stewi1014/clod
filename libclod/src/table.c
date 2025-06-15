#include <clod/table.h>
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define LF_MAX 0.85
#define LF_MIN 0.5
#define LF(table) ((double)((table)->elem_count + (table)->deleted_count) / (double)(table)->table_size)
// the size of the table needed to store capacity elements at the given load factor
#define LF_CAPACITY_TO_SIZE(lf, capacity) ((size_t)ceil((double)(capacity) / (lf)))

#define CTL_HASH_BITS 7
#define CTL_EMPTY             (0b00000000)
#define CTL_OCCUPIED(hash)    (0b10000000 | (uint8_t)(hash))
#define CTL_REMOVED	          (0b00000001)
#define CTL_IS_OCCUPIED(ctl) ((0b10000000 & (ctl)) > 0)

#define INDEX_NIL ((size_t)-1)

struct element {
	const void *element;
	size_t key_size;
};
struct clod_table {
	struct clod_table_opts opts;

	size_t elem_count;
	size_t deleted_count;
	size_t table_size;
	size_t cursor;

	uint8_t *restrict control;
	struct element *restrict elements;
};
static struct table_position {
	size_t index;
	uint8_t ctl;
}
get_position(const struct clod_table *t, const void *key, const size_t key_size) {
	assert(t->table_size > 0);
	auto const hash = t->opts.hash_func((size_t)t->control, key, key_size);

	return (struct table_position){
		.ctl = CTL_OCCUPIED(hash),
		.index = (hash >> CTL_HASH_BITS) % t->table_size
	};
}
static bool create(struct clod_table *t, const struct clod_table_opts *opts, const size_t new_table_size) {
	memcpy(&t->opts, opts, sizeof(t->opts));
	t->elem_count = 0;
	t->deleted_count = 0;
	t->table_size = new_table_size < t->opts.min_capacity ? t->opts.min_capacity : new_table_size;
	t->cursor = 0;

	if (t->table_size > 0) {
		t->elements = malloc(t->table_size * (sizeof(t->control[0]) + sizeof(t->elements[0])));
		if (!t->elements) {
			return false;
		}
		t->control = (uint8_t*)((char*)t->elements + t->table_size * sizeof(t->elements[0]));
		memset(t->control, 0, t->table_size);
	} else {
		t->control = nullptr;
		t->elements = nullptr;
	}

	return true;
}
static void destroy(const struct clod_table *t) {
	free(t->elements);
}
static bool key_equal(const struct clod_table *t, const size_t index, const void *key, const size_t key_size) {
	auto const res = t->elements[index];
	if (res.key_size != key_size) return false;
	return t->opts.cmp_func(res.element, key, res.key_size) == 0;
}
static struct probe {
	size_t existing;
	size_t available;
}
probe(const struct clod_table *t, const struct table_position pos, const void *key, const size_t key_size) {
	assert(pos.index < t->table_size);

	size_t available = INDEX_NIL;
	for (size_t i = 0; i < t->table_size; i++) {
		const size_t index = (pos.index + i) % t->table_size;

		if (t->control[index] == CTL_EMPTY) {
			return (struct probe){
				.existing = INDEX_NIL,
				.available = available != INDEX_NIL ? available : index
			};
		}

		if (t->control[index] == pos.ctl && key_equal(t, index, key, key_size)) {
			return (struct probe){
				.existing = index,
				.available = INDEX_NIL
			};
		}

		if (t->control[index] == CTL_REMOVED && available == INDEX_NIL) {
			available = index;
		}
	}

	return (struct probe){
		.existing = INDEX_NIL,
		.available = available
	};
}
static const void *insert(struct clod_table *t, const bool replace, const void *element, const size_t key_size) {
	assert(t->elem_count < t->table_size);

	auto const pos = get_position(t, element, key_size);
	auto const res = probe(t, pos, element, key_size);

	if (res.existing != INDEX_NIL) {
		if (replace) {
			const void *previous = t->elements[res.existing].element;
			t->elements[res.existing].element = element;
			t->elements[res.existing].key_size = key_size;
			return previous;
		} else {
			return t->elements[res.existing].element;
		}
	}

	assert(res.available != INDEX_NIL);
	t->elem_count++;
	if (t->control[res.available] == CTL_REMOVED) t->deleted_count--;
	t->control[res.available] = pos.ctl;
	t->elements[res.available].element = element;
	t->elements[res.available].key_size = key_size;
	return nullptr;
}
static bool rebuild(struct clod_table *t, const size_t new_table_size) {
	assert(new_table_size >= t->elem_count);

	struct clod_table new;
	if (!create(&new, &t->opts, new_table_size)) {
		return false;
	}

	auto iter = CLOD_TABLE_ITER_INIT;
	while (clod_table_iter(t, &iter)) {
		if (insert(&new, false, iter.element, iter.key_size) != nullptr) {
			destroy(&new);
			return false;
		}
	}

	destroy(t);
	memcpy(t, &new, sizeof(new));
	return true;
}
static void apply_default_opts(struct clod_table_opts *opts) {
	if (!opts->hash_func) opts->hash_func = clod_hash64;
	if (!opts->cmp_func) opts->cmp_func = memcmp;
}
struct clod_table *clod_table_create(const struct clod_table_opts *opts) {
	struct clod_table *t = malloc(sizeof(*t));
	if (!t) return nullptr;

	if (opts) memcpy(&t->opts, opts, sizeof(t->opts));
	else memset(&t->opts, 0, sizeof(t->opts));
	apply_default_opts(&t->opts);

	if (!create(t, &t->opts, LF_CAPACITY_TO_SIZE(LF_MAX, t->opts.min_capacity))) {
		free(t);
		return nullptr;
	}

	return t;
}
void clod_table_destroy(struct clod_table *t) {
	destroy(t);
	free(t);
}
size_t clod_table_len(const struct clod_table *t) {
	return t->elem_count;
}
void *clod_table_add(struct clod_table *t, const void *element, const size_t key_size) {
	if (t->table_size == 0 || LF(t) >= LF_MAX) {
		if (!rebuild(t, LF_CAPACITY_TO_SIZE(LF_MIN, t->elem_count + 1))) {
			return (void*)element;
		}
	}

	return (void*)insert(t, false, element, key_size);
}
void *clod_table_set(struct clod_table *t, const void *element, const size_t key_size) {
	if (t->table_size == 0 || LF(t) >= LF_MAX) {
		if (!rebuild(t, LF_CAPACITY_TO_SIZE(LF_MIN, t->elem_count + 1))) {
			return (void*)element;
		}
	}

	return (void*)insert(t, true, element, key_size);
}
void *clod_table_get(const struct clod_table *t, const void *key, const size_t key_size) {
	if (t->table_size == 0) return nullptr;
	auto const res = probe(t, get_position(t, key, key_size), key, key_size);
	if (res.existing != INDEX_NIL) {
		return (void*)t->elements[res.existing].element;
	}

	return nullptr;
}
void *clod_table_del(struct clod_table *t, const void *key, const size_t key_size) {
	if (t->table_size == 0) return nullptr;
	auto const res = probe(t, get_position(t, key, key_size), key, key_size);
	if (res.existing != INDEX_NIL) {
		t->control[res.existing] = CTL_REMOVED;
		t->elem_count--;
		t->deleted_count++;
		t->cursor = (t->cursor + 1) % t->table_size;
		return (void*)t->elements[res.existing].element;
	}

	return nullptr;
}
bool clod_table_iter(const struct clod_table *t, struct clod_table_iter *iter) {
	while (iter->_internal < t->table_size) {
		const size_t index = (iter->_internal + t->cursor) % t->table_size;
		if (CTL_IS_OCCUPIED(t->control[index])) {
			auto const res = t->elements[index];
			iter->element = (void*)res.element;
			iter->key_size = res.key_size;
			iter->_internal++;
			return true;
		}
		iter->_internal++;
	}
	iter->_internal = 0;
	iter->element = nullptr;
	iter->key_size = 0;
	return false;
}
