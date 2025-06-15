#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <clod/table.h>

#define STR "abcd"
#define LEN strlen(STR)

int main() {
	const char *val = STR;
	const char *val2 = STR;
	struct clod_table_iter iter;

	struct clod_table *t = clod_table_create(nullptr);
	check(clod_table_len(t) == 0);
	check(clod_table_get(t, val, LEN) == nullptr);
	check(clod_table_del(t, val, LEN) == nullptr);
	check(clod_table_add(t, val2, LEN) == nullptr);
	check(clod_table_add(t, val, LEN) == val2);
	check(clod_table_set(t, val, LEN) == val2);
	iter = CLOD_TABLE_ITER_INIT;
	check(clod_table_iter(t, &iter));
	check(iter.element == val);
	check(iter.key_size == LEN);
	check(!clod_table_iter(t, &iter));
	check(clod_table_get(t, STR, LEN) == val);
	check(clod_table_del(t, STR, LEN) == val);
	check(clod_table_get(t, STR, LEN) == nullptr);
	check(clod_table_del(t, STR, LEN) == nullptr);
	clod_table_destroy(t);

	return 0;
}
