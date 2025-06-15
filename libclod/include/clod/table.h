/**
 * @file clod/table.h
 */
#ifndef CLOD_TABLE_H
#define CLOD_TABLE_H

#include <clod/lib.h>
#include <clod/hash.h>
#include <stddef.h>
#include <stdint.h>

/**
 * A hash table implementation.
 * It's a pure set at heart, but since methods never return a pointer not supplied by the user,
 * it can be trivially extended to a key->value map by storing the value after the key.
 * The difference in wording between 'element' and 'key' reflects this.
 * If implementing a map, 'element' implies the key and trailing value data associated with it,
 * while 'key' simply implies the key value.
 * If instead a pure set is required - 'element' and 'key' become interchangeable.
 * I would recommend using a struct with the first field being the key and subsequent fields being the value,
 * although for maps with variable-length keys more manual intervention may be required.
 */
struct clod_table;

struct clod_table_opts {
	/** Minimum number of elements to support without further allocation.
	 * The table will never shrink to a capacity below this. */
	size_t min_capacity;
	/** Custom hash function.
	 * This map implementation demands a uniform spread of entropy across _all_ bits in uint64,
	 * and that two keys with nonequal hashes are also not equal when using the cmp_func method.*/
	uint64_t (*hash_func)(uint64_t seed, const void *key, size_t key_size);
	/** Custom equality function.
	 * The required behaviour for the hash_func and cmp_func relationship is;
	 * assert(cmp_func(a, b) != 0 || hash_func(a) == hash_func(b));
	 * In other words, if cmp_func thinks two elements are equal, hash_func must agree.*/
	int (*cmp_func)(const void *key1, const void *key2, size_t key_size);
};

/**
 * Create a new table.
 * @param[in] opts Configuration options for the table.
 * @return Initialised table.
 */
CLOD_API CLOD_USE_RETURN
struct clod_table *
clod_table_create(const struct clod_table_opts *opts);

/**
 * Release resources associated with the table.
 * @param t Table to free.
 */
CLOD_API CLOD_NONNULL(1)
void
clod_table_destroy(struct clod_table *t);

/**
 * Get the number of elements in the table.
 * @param[in] t Handle to the table.
 * @return Number of elements currently in the table.
 */
CLOD_API CLOD_NONNULL(1)
size_t
clod_table_len(const struct clod_table *t);

/**
 * Add an element.
 * If the key already exists, the operation will fail.
 *
 * @param[in] t Handle to the table.
 * @param[in] element Element to add. The table takes ownership on success.
 * @param[in] key_size Size of the key.
 * @return Nullptr on success, the existing element if the key exists, or the provided element on allocation failure.
 * The table retains ownership of an existing element.
 */
CLOD_API CLOD_NONNULL(1, 2)
void *
clod_table_add(struct clod_table *t, const void *element, size_t key_size);

/**
 * Add or replace an element.
 * If the key already exists, it is replaced.
 *
 * @param[in] t Handle to the table.
 * @param[in] element Element to insert. The table takes ownership on success.
 * @param[in] key_size Size of the key.
 * @return Previous element if the key existed, nullptr if it didn't, or the provided element on allocation failure.
 * The caller takes ownership of the previous element.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2)
void *
clod_table_set(struct clod_table *t, const void *element, size_t key_size);

/**
 * Get an element from the table.
 * @param[in] t Handle to the table.
 * @param[in] key Key to look up. Caller retains ownership.
 * @param[in] key_size Size of the key.
 * @return Found element or null. The table retains ownership of the element.
 */
CLOD_API CLOD_NONNULL(1, 2)
void *
clod_table_get(const struct clod_table *t, const void *key, size_t key_size);

/**
 * Remove an element from the table.
 * @param[in] t Handle to the table.
 * @param[in] key Key to delete. Caller retains ownership.
 * @param[in] key_size Size of the key.
 * @return Pointer to the removed element, or nullptr if the key doesn't exist.
 * The caller takes ownership of the element.
 */
CLOD_API CLOD_USE_RETURN CLOD_NONNULL(1, 2)
void *
clod_table_del(struct clod_table *t, const void *key, size_t key_size);

struct clod_table_iter {
	void *element;
	size_t key_size;
	size_t _internal;
};
#define CLOD_TABLE_ITER_INIT (struct clod_table_iter){ ._internal = 0 }

/**
 * Get the next element in, or start, an iteration over table elements.
 * Mutating the table during iteration can result in existing
 * elements being iterated more than once or not at all.
 *
 * The iterator should be zero initialised to start an iteration.
 * The iterator is zeroed at the end of the iteration.
 *
 * @param[in] t Handle to the table.
 * @param[in,out] iter Iterator to be incremented.
 * @return If the next element was found.
 */
CLOD_API CLOD_NONNULL(1, 2)
bool
clod_table_iter(const struct clod_table *t, struct clod_table_iter *iter);

#endif
