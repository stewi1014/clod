#pragma once

#include <stdint.h>
#include <string.h>

struct divl_t { int64_t quot; int64_t rem; };

/** division the way god intended */
inline struct divl_t divl(const int64_t n, const int64_t divisor) {
    int64_t quot = n / divisor;
    int64_t rem = n % divisor;

    if (rem != 0 && rem < 0 != divisor < 0) {
        rem += divisor;
        quot--;
    }

    return (struct divl_t){quot, rem};
}

/** n^p */
inline int64_t powl(const int64_t n, const int64_t p) {
    int64_t ret = 1;
    for (int64_t i = 0; i < p; i++) {
        ret *= n;
    }
    return ret;
}

/** Bubble Sort. */
inline void bsort(
    const void *restrict array,
    const size_t elem_size,
    const size_t n,
    void *user,
    int64_t (*cmp)(void *user, const void *, const void *)
) {
    char tmp[elem_size];
    int64_t unsorted = n;
    do {
        int64_t last_swap = 0;
        for (int64_t i = 1; i < unsorted; i++) {
            if (cmp(user, array + (i - 1) * elem_size, array + i * elem_size) < 0) {
                memcpy(tmp, array + i * elem_size, elem_size);
                memcpy(array + i * elem_size, array + (i - 1) * elem_size, elem_size);
                memcpy(array + (i - 1) * elem_size, tmp, elem_size);
                last_swap = i;
            }
        }
        unsorted = last_swap;
    } while (unsorted > 1);
}

inline bool vec_equal(const int64_t size, const int64_t *vec1, const int64_t *vec2) {
    for (int64_t i = 0; i < size; i++) {
        if (vec1[i] != vec2[i]) {
            return false;
        }
    }
    return true;
}

inline void vec_div(const int64_t size, int64_t *out, const int64_t *in, const int64_t divisor) {
    for (int64_t i = 0; i < size; i++) {
        out[i] = divl(in[i], divisor).quot;
    }
}

