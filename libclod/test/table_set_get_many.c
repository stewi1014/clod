#include "test.h"
#include <clod/table.h>
#include <stdlib.h>
#include <string.h>

#define NUM_ELEMS 100000

int main() {
	int elems[NUM_ELEMS];
	int elems2[NUM_ELEMS];
	for (int i = 0; i < NUM_ELEMS; i++) {
		elems[i] = i;
		elems2[i] = NUM_ELEMS + i;
	}

	struct clod_table *t = clod_table_create(nullptr);
	for (int i = 0; i < NUM_ELEMS; i++) {
		check(clod_table_add(t, &elems[i], 4) == nullptr);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check(clod_table_add(t, &elems[i], 4) == &elems[i]);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		const int *p = &i;
		check(clod_table_set(t, p, 4) == &elems[i]);
		check(clod_table_set(t, &elems[i], 4) == p);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check(clod_table_add(t, &elems2[i], 4) == nullptr);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check(clod_table_add(t, &elems[i], 4) == &elems[i]);
	}
	for (int i = 0; i < NUM_ELEMS - 1; i++) {
		check(clod_table_del(t, &i, 4) == &elems[i]);
	}
	for (int i = 0; i < NUM_ELEMS; i++) {
		check(clod_table_get(t, &elems2[i], 4) == &elems2[i]);
	}
	clod_table_destroy(t);

	return EXIT_SUCCESS;
}
