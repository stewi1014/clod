#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <clod/table.h>

#define STR "abcd"
#define LEN strlen(STR)

int main() {
    struct clod_table *t = clod_table_create(nullptr);

    const char *val = STR;

    assert(clod_table_set(t, val, LEN));

    auto iter = CLOD_TABLE_ITER_INIT;
    assert(clod_table_iter(t, &iter));
    assert(iter.element == val);
    assert(iter.key_size == LEN);
    assert(!clod_table_iter(t, &iter));

    assert(clod_table_get(t, STR, LEN) == val);
    assert(clod_table_del(t, STR, LEN) == val);
    assert(clod_table_get(t, STR, LEN) == nullptr);
    assert(clod_table_del(t, STR, LEN) == nullptr);
}
