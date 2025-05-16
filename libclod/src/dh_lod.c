#include <stdlib.h>

#include <anvil.h>
#include <nbt.h>
#include <dh.h>

#include "compress.h"
#include "dh_lod.h"

dh_result dh_lod_reset(
    struct dh_lod *lod,
    struct dh_lod_ext **ext_ptr
) {
    if (lod->realloc == nullptr) lod->realloc = realloc;

    lod->x = 0;
    lod->z = 0;
    lod->height = 0;
    lod->min_y = 0;
    lod->mip_level = 0;
    lod->compression_mode = 0;
    lod->mapping_len = 0;
    lod->lod_len = 0;
    lod->has_data = false;

    struct dh_lod_ext *ext = lod->__ext;
    if (ext == nullptr) {
        ext = lod->realloc(nullptr, sizeof(struct dh_lod_ext));
        if (ext == nullptr) {
            return DH_ERR_ALLOC;
        }

        ext->temp_string = nullptr;
        ext->temp_string_cap = 0;

        ext->temp_array = nullptr;
        ext->temp_array_cap = 0;

        ext->temp_buffer = nullptr;
        ext->temp_buffer_cap = 0;

        ext->big_buffer = nullptr;
        ext->big_buffer_cap = 0;

        ext->lzma_ctx = nullptr;
        ext->lz4_ctx = nullptr;

        for (int64_t i = 0; i < 4; i++) {
            ext->sections[i] = ANVIL_SECTIONS_CLEAR;
            ext->id_lookup[i] = ID_LOOKUP_CLEAR;
        }

        lod->__ext = ext;
    }

    *ext_ptr = ext;
    return DH_OK;
}

#define SIZE 2
#define NAME _2x2
#include "dh_lod_mip.c"
#undef SIZE
#undef NAME

#define SIZE 4
#define NAME _4x4
#include "dh_lod_mip.c"
#undef SIZE
#undef NAME

#define SIZE 8
#define NAME _8x8
#include "dh_lod_mip.c"
#undef SIZE
#undef NAME

#define SIZE 16
#define NAME _16x16
#include "dh_lod_mip.c"
#undef SIZE
#undef NAME

#define SIZE 32
#define NAME _32x32
#include "dh_lod_mip.c"
#undef SIZE
#undef NAME

#define SIZE 64
#define NAME _64x64
#include "dh_lod_mip.c"
#undef SIZE
#undef NAME

static
bool all_have_mip_level(
    const int64_t mip_level,
    const int64_t num_lods,
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
    const int64_t num_lods,
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
    const int64_t num_lods
) {
    if (
        lod == nullptr  ||
        lods == nullptr ||
        mip_level < 0   ||
        num_lods < 0
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

dh_result dh_lod_serialise_mapping(
    const struct dh_lod *lod,
    char **out,
    size_t *n_bytes
) {
    struct dh_lod_ext *ext = lod->__ext;
    *n_bytes = 0;

    #define ensure_buffer(n) \
        if (ext->temp_buffer_cap < (n) + (*n_bytes)) {\
            size_t new_cap = (n) + ext->temp_buffer_cap * 2;\
            char *new = lod->realloc(ext->temp_buffer, new_cap);\
            if (new == nullptr) {\
                return DH_ERR_ALLOC;\
            }\
            ext->temp_buffer_cap = new_cap;\
            ext->temp_buffer = new;\
        }\

    ensure_buffer(4);
    ext->temp_buffer[(*n_bytes)++] = (char)((lod->mapping_len >> (2 * 8)) & 0xFF);
    ext->temp_buffer[(*n_bytes)++] = (char)((lod->mapping_len >> (3 * 8)) & 0xFF);
    ext->temp_buffer[(*n_bytes)++] = (char)((lod->mapping_len >> (1 * 8)) & 0xFF);
    ext->temp_buffer[(*n_bytes)++] = (char)((lod->mapping_len >> (0 * 8)) & 0xFF);
    
    for (int64_t i = 0; i < lod->mapping_len; i++) {
        const size_t size = strlen(lod->mapping_arr[i]);
        if (size > UINT16_MAX) {
            return DH_ERR_MALFORMED;
        }

        ensure_buffer(2 + size);
        ext->temp_buffer[(*n_bytes)++] = (char)((size >> (1 * 8)) & 0xFF);
        ext->temp_buffer[(*n_bytes)++] = (char)((size >> (0 * 8)) & 0xFF);
        memcpy(&ext->temp_buffer[*n_bytes], lod->mapping_arr[i], size);
        *n_bytes += size;
    }

    #undef ensure_buffer

    switch (lod->compression_mode) {
    case DH_DATA_COMPRESSION_UNCOMPRESSED: {
        *out = ext->temp_buffer;
        return DH_OK;
    }
    case DH_DATA_COMPRESSION_LZ4: {
        size_t compressed_mapping_len;

        const int result = compress_lz4(
            &ext->lz4_ctx,
            ext->temp_buffer,
            *n_bytes,
            &ext->temp_string,
            &ext->temp_string_cap,
            &compressed_mapping_len,
            lod->realloc
        );

        if (result != 0) {
            *n_bytes = 0;
            return DH_ERR_COMPRESS;
        }

        *out = ext->temp_string;
        *n_bytes = compressed_mapping_len;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_LZMA2: {
        size_t compressed_mapping_len;

        const lzma_ret result = compress_lzma(
            &ext->lzma_ctx,
            ext->temp_buffer,
            *n_bytes,
            &ext->temp_string,
            &ext->temp_string_cap,
            &compressed_mapping_len,
            lod->realloc
        );

        if (result != LZMA_OK && result != LZMA_STREAM_END) {
            *n_bytes = 0;
            return DH_ERR_COMPRESS;
        }

        *out = ext->temp_string;
        *n_bytes = compressed_mapping_len;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_ZSTD: {
        return DH_ERR_UNSUPPORTED;
    }
    default: {
        return DH_ERR_INVALID_ARGUMENT;
    }
    }
}

dh_result dh_compress(
    struct dh_lod *lod,
    const int64_t compression_mode,
    double level
) {
    struct dh_lod_ext *ext = lod->__ext;

    if (compression_mode == lod->compression_mode)
        return DH_OK;
    
    char  *decompressed_lod_arr;
    size_t decompressed_lod_len;
    size_t decompressed_lod_cap;

    // existing format -> decompressed
    // this step consumes the data in the LOD,
    // writing the uncompressed data into the decompressed buffer.
    // after this step, the LOD's data and mapping is assumed to be invalid.
    // 

    switch (lod->compression_mode) {
    case DH_DATA_COMPRESSION_UNCOMPRESSED: {
        decompressed_lod_arr = lod->lod_arr;
        decompressed_lod_len = lod->lod_len;
        decompressed_lod_cap = lod->lod_cap;
        break;
    }
    case DH_DATA_COMPRESSION_LZ4: {
        return DH_ERR_UNSUPPORTED;
    }
    case DH_DATA_COMPRESSION_LZMA2: {
        return DH_ERR_UNSUPPORTED;
    }
    case DH_DATA_COMPRESSION_ZSTD: {
        return DH_ERR_UNSUPPORTED;
    }
    default: {
        return DH_ERR_INVALID_ARGUMENT;
    }
    }


    switch (compression_mode) {
    case DH_DATA_COMPRESSION_UNCOMPRESSED: {
        lod->lod_arr = decompressed_lod_arr;
        lod->lod_len = decompressed_lod_len;
        lod->lod_cap = decompressed_lod_cap;

        lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_LZ4: {
        size_t compressed_lod_len;

        const int result = compress_lz4(
            &ext->lz4_ctx,
            decompressed_lod_arr,
            decompressed_lod_len,
            &ext->big_buffer,
            &ext->big_buffer_cap,
            &compressed_lod_len,
            lod->realloc
        );

        if (result != 0) {
            lod->lod_arr = decompressed_lod_arr;
            lod->lod_len = decompressed_lod_len;
            lod->lod_cap = decompressed_lod_cap;
    
            lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;
    
            return DH_ERR_COMPRESS;
        }

        lod->lod_arr = ext->big_buffer;
        lod->lod_len = compressed_lod_len;
        lod->lod_cap = ext->big_buffer_cap;

        ext->big_buffer = decompressed_lod_arr;
        ext->big_buffer_cap = decompressed_lod_cap;

        lod->compression_mode = DH_DATA_COMPRESSION_LZ4;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_LZMA2: {
        size_t compressed_lod_len;

        const lzma_ret result = compress_lzma(
            &ext->lzma_ctx,
            decompressed_lod_arr,
            decompressed_lod_len,
            &ext->big_buffer,
            &ext->big_buffer_cap,
            &compressed_lod_len,
            lod->realloc
        );

        if (result != LZMA_OK && result != LZMA_STREAM_END) {
            lod->lod_arr = decompressed_lod_arr;
            lod->lod_len = decompressed_lod_len;
            lod->lod_cap = decompressed_lod_cap;
    
            lod->compression_mode = DH_DATA_COMPRESSION_UNCOMPRESSED;
    
            return DH_ERR_COMPRESS;
        }

        lod->lod_arr = ext->big_buffer;
        lod->lod_len = compressed_lod_len;
        lod->lod_cap = ext->big_buffer_cap;

        ext->big_buffer = decompressed_lod_arr;
        ext->big_buffer_cap = decompressed_lod_cap;

        lod->compression_mode = DH_DATA_COMPRESSION_LZMA2;

        return DH_OK;
    }
    case DH_DATA_COMPRESSION_ZSTD: {
        return DH_ERR_UNSUPPORTED;
    }
    default: {
        return DH_ERR_INVALID_ARGUMENT;
    }
    }
}

void dh_lod_free(
    struct dh_lod *lod
) {
    if (lod == nullptr) return;
    if (lod->realloc == nullptr) return;

    for (int64_t i = 0; i < lod->mapping_cap; i++) lod->realloc(lod->mapping_arr[i], 0);
    lod->realloc(lod->mapping_arr, 0);

    lod->realloc(lod->lod_arr, 0);

    struct dh_lod_ext *ext = lod->__ext;
    if (ext != nullptr) {
        if (ext->temp_string != nullptr) lod->realloc(ext->temp_string, 0);
        if (ext->temp_array != nullptr) lod->realloc(ext->temp_array, 0);
        if (ext->temp_buffer != nullptr) lod->realloc(ext->temp_buffer, 0);
        if (ext->big_buffer != nullptr) lod->realloc(ext->big_buffer, 0);

        for (int64_t i = 0; i < 4; i++)
            anvil_sections_free(&ext->sections[i]);
        
        for (int64_t i = 0; i < 4; i++) {
            if (ext->id_lookup[i].sections != nullptr) {
                for (int64_t j = 0; j < ext->id_lookup->sections_cap; j++) {
                    lod->realloc(ext->id_lookup[i].sections[j].ids, 0);
                }
                lod->realloc(ext->id_lookup[i].sections, 0);
            }
        }

        if (ext->lzma_ctx != nullptr) compress_free_lzma(&ext->lzma_ctx, lod->realloc);
        if (ext->lz4_ctx != nullptr) compress_free_lz4(&ext->lz4_ctx, lod->realloc);
    }
    
    lod->__ext = nullptr;
    lod->realloc = nullptr;
}
