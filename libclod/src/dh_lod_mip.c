#include <stdint.h>
#include <dh.h>

#define DH_MIP_LEVELS 1
#define DH_MIP_NAME _2x2
#include "dh_lod_mip_nxn.c"
#undef DH_MIP_LEVELS
#undef DH_MIP_NAME

#define DH_MIP_LEVELS 2
#define DH_MIP_NAME _4x4
#include "dh_lod_mip_nxn.c"
#undef DH_MIP_LEVELS
#undef DH_MIP_NAME

#define DH_MIP_LEVELS 3
#define DH_MIP_NAME _8x8
#include "dh_lod_mip_nxn.c"
#undef DH_MIP_LEVELS
#undef DH_MIP_NAME

#define DH_MIP_LEVELS 4
#define DH_MIP_NAME _16x16
#include "dh_lod_mip_nxn.c"
#undef DH_MIP_LEVELS
#undef DH_MIP_NAME

#define DH_MIP_LEVELS 5
#define DH_MIP_NAME _32x32
#include "dh_lod_mip_nxn.c"
#undef DH_MIP_LEVELS
#undef DH_MIP_NAME

#define DH_MIP_LEVELS 6
#define DH_MIP_NAME _64x64
#include "dh_lod_mip_nxn.c"
#undef DH_MIP_LEVELS
#undef DH_MIP_NAME

static
bool all_have_mip_level(
    const int64_t mip_level,
    const size_t num_lods,
    struct dh_lod **lods
) {
    for (auto i = 0; i < num_lods; i++) {
        if (lods[i]->mip_level != mip_level) return false;
    }
    return true;
}

static
bool all_have_min_y(
    const int64_t min_y,
    const size_t num_lods,
    struct dh_lod **lods
) {
    for (auto i = 0; i < num_lods; i++) {
        if (lods[i]->min_y != min_y) return false;
    }
    return true;
}

dh_result dh_lod_mip(
    struct dh_lod *lod,
    const int64_t mip_level,
    struct dh_lod **lods,
    const size_t num_lods
) {
    if (
        lod == nullptr  ||
        lods == nullptr ||
        mip_level < 0
    ) return DH_ERR_INVALID_ARGUMENT;

    if (
        num_lods == 2 * 2 &&
        all_have_mip_level(mip_level - 1, num_lods, lods) &&
        all_have_min_y(lods[0]->min_y, num_lods, lods)
    ) {
        return dh_lod_mip_2x2(lod, lods);
    }

    if (
        num_lods == 4 * 4 &&
        all_have_mip_level(mip_level - 2, num_lods, lods) &&
        all_have_min_y(lods[0]->min_y, num_lods, lods)
    ) {
        return dh_lod_mip_4x4(lod, lods);
    }

    if (
        num_lods == 8 * 8 &&
        all_have_mip_level(mip_level - 3, num_lods, lods) &&
        all_have_min_y(lods[0]->min_y, num_lods, lods)
    ) {
        return dh_lod_mip_8x8(lod, lods);
    }

    if (
        num_lods == 16 * 16 &&
        all_have_mip_level(mip_level - 4, num_lods, lods) &&
        all_have_min_y(lods[0]->min_y, num_lods, lods)
    ) {
        return dh_lod_mip_16x16(lod, lods);
    }

    if (
        num_lods == 32 * 32 &&
        all_have_mip_level(mip_level - 5, num_lods, lods) &&
        all_have_min_y(lods[0]->min_y, num_lods, lods)
    ) {
        return dh_lod_mip_32x32(lod, lods);
    }

    if (
        num_lods == 64 * 64 &&
        all_have_mip_level(mip_level - 6, num_lods, lods) &&
        all_have_min_y(lods[0]->min_y, num_lods, lods)
    ) {
        return dh_lod_mip_64x64(lod, lods);
    }

    return DH_ERR_UNSUPPORTED;
}
