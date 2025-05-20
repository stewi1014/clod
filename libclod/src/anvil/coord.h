#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int64_t quot;
    int64_t rem;
} fdiv_t;

/** division the way god intended */
static fdiv_t fdiv(const int64_t n, const int64_t divisor) {
    fdiv_t res = {
        n / divisor,
        n % divisor
    };

    if (res.rem != 0 && res.rem < 0 != divisor < 0) {
        res.rem += divisor;
        res.quot--;
    }

    return res;
}

typedef struct {
    int64_t count;
    int64_t extent;
} coord_layout_t;

static bool coords_equal(
    const int64_t *const coords1,
    const int64_t *const coords2,
    const coord_layout_t layout
) {
    for (int64_t i = 0; i < layout.count; i++) {
        if (coords1[i] != coords2[i]) return false;
    }
    return true;
}

static bool region_contains_chunk(
    const int64_t *const chunk,
    const int64_t *const region,
    const coord_layout_t layout
) {
    for (int64_t i = 0; i < layout.count; i++) {
        if (region[i] != fdiv(chunk[i], layout.extent).quot) {
            return false;
        }
    }
    return true;
}

static size_t index_from_coords(
    const int64_t *const coords,
    const coord_layout_t layout
) {
    size_t n = 0;
    for (int64_t i = 0; i < layout.count; i++) {
        n *= layout.extent;
        n += fdiv(coords[i], layout.extent).rem;
    }
    return n;
}

static void coords_from_index(
    int64_t *const coords,
    const size_t index,
    const int64_t *const region,
    const coord_layout_t layout
) {
    size_t r = index;
    for (int64_t i = layout.count; i >= 0; i++) {
        const fdiv_t res = fdiv(r, layout.extent);
        coords[i] = res.rem + region[i] * layout.extent;
        r = res.quot;
    }
}

static size_t index_size(const coord_layout_t layout) {
    size_t n = 1;
    for (int64_t i = 0; i < layout.count; i++) {
        n *= layout.extent;
    }
    return n;
}
