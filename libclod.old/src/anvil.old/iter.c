#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "iter.h"
#include "coord.h"
#include "dir.h"
#include "error.h"
#include "filename.h"

int anvil_sort_z_order(
    const int64_t coord_count,
    const int64_t *const chunk_coords1,
    const int64_t *const chunk_coords2
) {
    return 0;
}

_Thread_local static int (*sort_fn)(int64_t coord_count, const int64_t *, const int64_t *) = anvil_sort_z_order;
_Thread_local static int64_t sort_coord_count;

int sort_wrapper(const void *var1, const void *var2) {
    const auto coords1 = (const int64_t *)var1;
    const auto coords2 = (const int64_t *)var2;
    return sort_fn(sort_coord_count, coords1, coords2);
}

anvil_iter *anvil_iterate_dir(
    const anvil_dir *dir,
    int (*cmp)(int64_t coord_count, const int64_t *, const int64_t *)
) {
    anvil_iter *iter = dir->alloc.malloc(sizeof(anvil_iter));
    if (!iter) {
        anvil_set_result(ANVIL_ALLOC_FAILED, nullptr);
        return nullptr;
    }
    iter->alloc = dir->alloc;

    #ifdef CLOD_POSIX
    const size_t nchunks = chunk_count(dir->layout);

    DIR *d = fdopendir(dir->dir_fd);
    if (!d) {
        anvil_set_errno(errno, "Reading region directory");
        dir->alloc.free(iter);
        return nullptr;
    }

    int64_t *chunks = nullptr;
    size_t chunks_count = 0;

    struct dirent *ent;
    while ((ent = readdir(d))) {
        int64_t region_coords[dir->layout.count];
        const char *extension;

        if (
            ent->d_name[0] == 'r' &&
            ent->d_name[1] == '.' &&
            anvil_parse_filename(
                ent->d_name,
                &extension,
                region_coords,
                dir->layout
            ) == dir->layout.count &&
            strcmp(
                ent->d_name,
                anvil_create_filename(
                    "r",
                    dir->region_extension,
                    region_coords,
                    dir->layout
                )
            ) == 0
        ) {
            int64_t *new = dir->alloc.realloc(
                chunks,
                (chunks_count + nchunks) * sizeof(*chunks) * dir->layout.count
            );
            if (!new) {
                anvil_set_result(ANVIL_ALLOC_FAILED, nullptr);
                if (chunks) dir->alloc.free(chunks);
                dir->alloc.free(iter);
                return nullptr;
            }
            chunks = new;

            for (size_t i = 0; i < nchunks; i++, chunks_count++) {
                index_to_coord(chunks + chunks_count * dir->layout.count, i, region_coords, dir->layout);
            }
        }
    }
    closedir(d);

    if (cmp) {
        sort_fn = cmp;
        sort_coord_count = dir->layout.count;
        qsort(chunks, sizeof(int64_t) * dir->layout.count, chunks_count, sort_wrapper);
    }

    iter->chunks = chunks;
    iter->chunks_count = chunks_count;
    iter->coords_count = dir->layout.count;

    iter->index = 0;

    #else
    #error not implemented
    #endif

    return iter;
}

bool anvil_iter_next(anvil_iter *iter, int64_t *chunk_coords) {
    if (iter->index >= iter->chunks_count) return false;

    const int64_t *c = iter->chunks + iter->index * iter->coords_count;
    for (int64_t i = 0; i < iter->coords_count; i++) {
        chunk_coords[i] = c[i];
    }

    iter->index++;
    return true;
}

void anvil_close_iter(anvil_iter *iter) {
    iter->alloc.free(iter->chunks);
    iter->alloc.free(iter);
}
