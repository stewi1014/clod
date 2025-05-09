#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <anvil.h>
#include <dh.h>

int main(int argc, char **argv) {
    struct anvil_world *world = anvil_open("world");
    if (world == NULL) {
        printf("open world: %s\n", strerror(errno));
        return -1;
    }
    
    struct anvil_chunk_ctx *chunk_ctx[16];
    for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++) {
        chunk_ctx[xi * 4 + zi] = anvil_chunk_ctx_alloc(NULL);
    }

    struct anvil_region_iter *iter = anvil_region_iter_new("region", world);
    struct anvil_region region;
    struct anvil_chunk chunks[16];
    struct dh_lod lod = DH_LOD_CLEAR;

    int error;
    while (!(error = anvil_region_iter_next(&region, iter))) {
        for (int x = 0; x < 32; x += 4) for (int z = 0; z < 32; z += 4) {
            for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++)
                chunks[xi * 4 + zi] = anvil_chunk_decompress(chunk_ctx[xi * 4 + zi], &region, x + xi, z + zi);

            dh_result result = dh_from_chunks(chunks, &lod);
            if (result != DH_OK) {
                printf("failed to create LOD for pos %d, %d\n", x, z);
                continue;
            }

            // do something with LOD...
        }        
    }
    if (error < 0) {
        printf("%s\n", strerror(errno));
        return -1;
    }
    
    for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++) {
        anvil_chunk_ctx_free(chunk_ctx[xi * 4 + zi]);
    }

    dh_lod_free(&lod);
    anvil_region_iter_free(iter);
    anvil_close(world);

    return 0;
}
