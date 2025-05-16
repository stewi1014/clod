#include <stdlib.h>

#include <anvil.h>
#include <nbt.h>
#include <dh.h>

#include "compress.h"
#include "dh_lod.h"

int dh_compare_strings(const void *str1_ptr, const void *str2_ptr) {
    const char *str1 = *(char**)str1_ptr; const char *str2 = *(char**)str2_ptr;
    if (str1 == nullptr && str2 == nullptr) return 0;
    if (str1 == nullptr) return 1;
    if (str2 == nullptr) return -1;

    return strcmp(str1, str2);
}

int dh_compare_lod_pos(const void *lod1_ptr, const void *lod2_ptr) {
    const struct dh_lod *lod1 = (struct dh_lod*)lod1_ptr;
    const struct dh_lod *lod2 = (struct dh_lod*)lod2_ptr;

    if (lod1 == nullptr && lod2 == nullptr) return 0;
    if (lod1 == nullptr) return 1;
    if (lod2 == nullptr) return -1;

    if (lod1->mip_level != lod2->mip_level) {
        return (int)(lod2->mip_level - lod1->mip_level);
    }

    if (lod1->x != lod2->x) {
        return (int)(lod1->x - lod2->x);
    }

    if (lod1->z != lod2->z) {
        return (int)(lod1->z - lod2->z);
    }

    return 0;
}

int64_t dh_lods_same_mip_level(struct dh_lod **lods, const size_t num_lods) {
    size_t i = 0;
    while (i < num_lods && lods[i] == NULL) i++;
    if (i == num_lods) return DH_LODS_NO_LODS;
    const int64_t mip_level = lods[i]->mip_level;
    i++;
    while (i < num_lods) {
        if (lods[i] != NULL && lods[i]->mip_level != mip_level)
            return DH_LODS_NOT_SAME;
        i++;
    }
    return mip_level;
}

int64_t dh_lods_same_min_y(struct dh_lod **lods, const size_t num_lods) {
    size_t i = 0;
    while (i < num_lods && lods[i] == NULL) i++;
    if (i == num_lods) return DH_LODS_NO_LODS;
    const int64_t min_y = lods[i]->min_y;
    i++;
    while (i < num_lods) {
        if (lods[i] != NULL && lods[i]->min_y != min_y)
            return DH_LODS_NOT_SAME;
        i++;
    }
    return min_y;
}

int64_t dh_lods_max_height(struct dh_lod **lods, const size_t num_lods) {
    int64_t height = DH_LODS_NO_LODS;
    for (size_t i = 0; i < num_lods; i++)
        if (lods[i] != NULL && lods[i]->height > height)
            height = lods[i]->height;
    return height;
}

dh_result dh_lod_ext_get(
    struct dh_lod *lod,
    struct dh_lod_ext **ext_ptr
) {
    if (lod->realloc == nullptr) lod->realloc = realloc;
    struct dh_lod_ext *ext = lod->__internal;
    if (ext == nullptr) {
        ext = lod->realloc(nullptr, sizeof(struct dh_lod_ext));
        if (ext == nullptr) {
            return DH_ERR_ALLOC;
        }

        *ext = DH_LOD_EXT_CLEAR;
        lod->__internal = ext;
    }

    *ext_ptr = ext;
    return DH_OK;
}

void dh_lod_ext_free(struct dh_lod *lod) {
    if (lod->realloc == nullptr) return;
    struct dh_lod_ext *ext = lod->__internal;
    if (ext == nullptr) return;

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

    lod->realloc(ext, 0);
    lod->__internal = nullptr;
}

dh_result dh_lod_merge_mappings(
    struct dh_lod *dst,
    struct dh_lod *src,
    uint32_t **id_mapping_ptr
) {
    if (dst == nullptr || src == nullptr)
        return DH_ERR_INVALID_ARGUMENT;

    struct dh_lod_ext *dst_ext, *src_ext;

    dh_result res = dh_lod_ext_get(dst, &dst_ext);
    if (res != DH_OK) return res;

    res = dh_lod_ext_get(src, &src_ext);
    if (res != DH_OK) return res;

    const size_t id_mapping_len = src->mapping_len * sizeof(uint32_t);
    if (src_ext->temp_string_cap < id_mapping_len) {
        char *new = src->realloc(src_ext->temp_string, id_mapping_len);
        if (new == nullptr) return DH_ERR_ALLOC;

        src_ext->temp_string_cap = id_mapping_len;
        src_ext->temp_string = new;
    }
    const auto id_mapping = (uint32_t*)src_ext->temp_string;
    *id_mapping_ptr = (uint32_t*)src_ext->temp_string;

    for (size_t i = 0; i < src->mapping_len; i++) {
        res = dh_lod_add_mapping(
            dst,
            src->mapping_arr[i],
            strlen(src->mapping_arr[i]),
            &id_mapping[i]
        );

        if (res != DH_OK) return res;
    }

    return DH_OK;
}

dh_result dh_lod_add_mapping(
    struct dh_lod *lod,
    const char *mapping,
    const size_t mapping_len,
    uint32_t *id_ptr
) {
    uint32_t id = 0;
    while (id < lod->mapping_len && strcmp(lod->mapping_arr[id], mapping) != 0)
        id++;

    if (id == lod->mapping_len) {
        if (lod->mapping_cap == lod->mapping_len) {
            const size_t new_cap = lod->mapping_cap == 0 ? 16 : lod->mapping_cap * 2;
            char **new = lod->realloc(lod->mapping_arr, new_cap * sizeof(char*));
            if (new == nullptr) return DH_ERR_ALLOC;

            for (size_t i = lod->mapping_cap; i < new_cap; i++) new[i] = nullptr;
            lod->mapping_arr = new;
            lod->mapping_cap = new_cap;
        }

        char *new = lod->realloc(lod->mapping_arr[id], mapping_len);
        if (new == nullptr) return DH_ERR_ALLOC;

        strncpy(new, mapping, mapping_len);
        lod->mapping_arr[id] = new;
        lod->mapping_len++;
    }

    *id_ptr = id;
    return DH_OK;
}

dh_result dh_lod_ensure(
    struct dh_lod *lod,
    const size_t n
) {
    if (lod->lod_cap < lod->lod_len + n) {
        size_t new_cap = (lod->lod_cap << 1) - (lod->lod_cap >> 1);
        if (new_cap < lod->lod_len + n) new_cap = lod->lod_len + n;

        char *new = lod->realloc(lod->lod_arr, new_cap * sizeof(char));
        if (new == nullptr) {
            return DH_ERR_ALLOC;
        }

        lod->lod_arr = new;
        lod->lod_cap = new_cap;
    }

    return DH_OK;
}

dh_result dh_lod_serialise_mapping(
    struct dh_lod *lod,
    char **out,
    size_t *n_bytes
) {
    struct dh_lod_ext *ext;
    const dh_result res = dh_lod_ext_get(lod, &ext);
    if (res != DH_OK) return res;

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
            lod->realloc,
            1
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
            lod->realloc,
            1
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
    const double level
) {
    struct dh_lod_ext *ext;
    const dh_result res = dh_lod_ext_get(lod, &ext);
    if (res != DH_OK) return res;

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
            lod->realloc,
            level
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
            lod->realloc,
            level
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

void dh_lod_trim(
    struct dh_lod *lod
) {
    if (lod == nullptr) return;
    if (lod->realloc == nullptr) return;

    for (size_t i = lod->mapping_len; i < lod->mapping_cap; i++) if (lod->mapping_arr[i] != nullptr) {
        lod->realloc(lod->mapping_arr[i], 0);
        lod->mapping_arr[i] = nullptr;
    }

    if (lod->mapping_cap > lod->mapping_len) {
        char **new = lod->realloc(lod->mapping_arr, lod->mapping_len * sizeof(*lod->mapping_arr));
        if (new != nullptr || lod->mapping_len == 0) {
            lod->mapping_arr = new;
            lod->mapping_cap = lod->mapping_len;
        }
    }

    if (lod->lod_cap > lod->lod_len) {
        char *new = lod->realloc(lod->lod_arr, lod->lod_len);
        if (new != nullptr || lod->lod_len == 0) {
            lod->lod_arr = new;
            lod->lod_cap = lod->lod_len;
        }
    }

    dh_lod_ext_free(lod);
}

void dh_lod_free(
    struct dh_lod *lod
) {
    if (lod == nullptr) return;
    if (lod->realloc == nullptr) return;

    for (int64_t i = 0; i < lod->mapping_cap; i++) {
        if (lod->mapping_arr[i] != nullptr)
            lod->realloc(lod->mapping_arr[i], 0);
        lod->mapping_arr[i] = nullptr;
    }

    if (lod->mapping_arr != nullptr)
        lod->realloc(lod->mapping_arr, 0);

    if (lod->lod_arr != nullptr)
        lod->realloc(lod->lod_arr, 0);

    dh_lod_ext_free(lod);

    *lod = DH_LOD_CLEAR;
}
