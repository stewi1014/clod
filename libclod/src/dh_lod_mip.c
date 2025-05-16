#include <stdlib.h>

#include <dh.h>

#include "dh_lod.h"

dh_result dh_from_lods(
    struct dh_lod *lods, // 2x2 array of source LODs.
    struct dh_lod *lod  // destination LOD.
) {
    if (lods == nullptr || lod == nullptr) return DH_ERR_INVALID_ARGUMENT;
    for (int64_t i = 0; i < 4; i++) if (lods[i].realloc == nullptr) lods[i].realloc = realloc;
    if (lod->realloc == nullptr) lod->realloc = realloc;

    return DH_ERR_ALLOC;
}
