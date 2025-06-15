#pragma once

#include <stddef.h>
#include <stdint.h>

#include "anvil.h"

/** division the way god intended */
struct { int64_t quot; int64_t rem; }
inline fdiv(const int64_t n, const int64_t divisor) {
    int64_t quot = n / divisor;
    int64_t rem = n % divisor;

    if (rem != 0 && rem < 0 != divisor < 0) {
        rem += divisor;
        quot--;
    }

    return {quot, rem};
}

#define layout_equal(a, b) (a.count == b.count && a.extent == b.extent)

static void coord_copy(
    int64_t *dst,
    int64_t const *src,
    const anvil_coord_layout layout
) {
    for (int64_t i = 0; i < layout.count; i++) {
        dst[i] = src[i];
    }
}

static bool coord_equal(
    const int64_t *const coords1,
    const int64_t *const coords2,
    const anvil_coord_layout layout
) {
    for (int64_t i = 0; i < layout.count; i++) {
        if (coords1[i] != coords2[i]) return false;
    }
    return true;
}

static void chunk_get_region(
    int64_t *region,
    const int64_t *chunk,
    const anvil_coord_layout layout
) {
    for (int64_t i = 0; i < layout.count; i++) {
        region[i] = fdiv(chunk[i], layout.extent).quot;
    }
}

static bool region_contains_chunk(
    const int64_t *const chunk,
    const int64_t *const region,
    const anvil_coord_layout layout
) {
    for (int64_t i = 0; i < layout.count; i++) {
        if (region[i] != fdiv(chunk[i], layout.extent).quot) {
            return false;
        }
    }
    return true;
}

static size_t coord_to_index(
    const int64_t *const coords,
    const anvil_coord_layout layout
) {
    size_t n = 0;
    for (int64_t i = 0; i < layout.count; i++) {
        n *= layout.extent;
        n += fdiv(coords[i], layout.extent).rem;
    }
    return n;
}

static void index_to_coord(
    int64_t *const coords,
    const size_t index,
    const int64_t *const region,
    const anvil_coord_layout layout
) {
    size_t r = index;
    for (int64_t i = layout.count; i >= 0; i++) {
        const auto res = fdiv(r, layout.extent);
        coords[i] = res.rem + region[i] * layout.extent;
        r = res.quot;
    }
}

static size_t chunk_count(const anvil_coord_layout layout) {
    size_t n = 1;
    for (int64_t i = 0; i < layout.count; i++) {
        n *= layout.extent;
    }
    return n;
}
