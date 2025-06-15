#include <stddef.h>
#include <stdint.h>
#include <alloca.h>
#include <string.h>

/** n^p */
static int64_t pow64(const int64_t n, const int64_t p) {
    int64_t ret = 1;
    for (int64_t i = 0; i < p; i++) {

        ret *= n;
    }
    return ret;
}

struct divl_t { int64_t quot; int64_t rem; };

/** division the way god intended */
/*
static struct divl_t div64(const int64_t n, const int64_t divisor) {
    int64_t quot = n / divisor;
    int64_t rem = n % divisor;

    if (rem != 0 && ((rem < 0) != (divisor < 0))) {
        rem += divisor;
        quot--;
    }

    return (struct divl_t){quot, rem};
}
*/

/** Bubble Sort. */
/*
static void bsort(
    void *array,
    const size_t elem_size,
    const size_t n,
    void *user,
    int64_t (*cmp)(void *user, const void *, const void *)
) {
    uint8_t *t = alloca(elem_size);
    size_t unsorted = n;
    do {
        size_t last_swap = 0;
        for (size_t i = 1; i < unsorted; i++) {
            uint8_t *const a = (uint8_t*)array + elem_size * (i - 1);
            uint8_t *const b = (uint8_t*)array + elem_size * i;

            if (cmp(user, a, b) < 0) {
                memcpy(t, b, elem_size);
                memcpy(b, a, elem_size);
                memcpy(a, t, elem_size);
                last_swap = i;
            }
        }
        unsorted = last_swap;
    } while (unsorted > 1);
}
*/
/*

static int64_t *vec_div(int64_t *out, const int64_t *in, const int64_t size, const int64_t divisor) {
    for (int64_t i = 0; i < size; i++) {
        out[i] = div64(in[i], divisor).quot;
    }
    return out;
}

#define get_region_pos(chunk_pos, opts) vec_div(\
    alloca(sizeof(int64_t) * (size_t)opts.coord_count),\
    chunk_pos,\
    opts.coord_count,\
    opts.region_extent\
)

*/