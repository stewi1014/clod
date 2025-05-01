#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <anvil.h>
#include <nbt.h>

int main(int argc, char **argv) {
    struct anvil_world *world = anvil_open("world");
    if (world == NULL) {
        printf("open world: %s\n", strerror(errno));
        return -1;
    }

    struct anvil_chunk_ctx *chunk_ctx = anvil_chunk_ctx_alloc(NULL);
    struct anvil_region_iter *iter = anvil_region_iter_new("region", world); // e.g. region, DIM1, DIM-1.
    struct anvil_region region;
    int error;
    while (!(error = anvil_region_iter_next(&region, iter))) {
        struct anvil_chunk chunk;
        for (int x = 0; x < 32; x++) for (int z = 0; z < 32; z++) {
            chunk = anvil_chunk_decompress(chunk_ctx, &region, x, z);
            if (chunk.data_size == 0) continue; // files often have empty chunks.
    
            char *status = nbt_named(chunk.data, chunk.data + chunk.data_size, "Status");
            if (status == NULL && nbt_type(status) != NBT_STRING) {
                printf(
                    "region (%d, %d); chunk (%d, %d) is corrupted\n", 
                    region.region_x,
                    region.region_z,
                    chunk.chunk_x,
                    chunk.chunk_z
                );
            } else {
                printf(
                    "region (%d, %d), chunk (%d, %d) has status %.*s\n", 
                    region.region_x,
                    region.region_z,
                    chunk.chunk_x,
                    chunk.chunk_z,
                    nbt_string_size(nbt_payload(status, NBT_STRING)),
                    nbt_string(nbt_payload(status, NBT_STRING))
                );
            }

            // use chunk NBT data
        }
    }
    
    if (error < 0) {
        printf("%s\n", strerror(errno));
        return -1;
    }
    
    anvil_chunk_ctx_free(chunk_ctx);
    anvil_region_iter_free(iter);
    anvil_close(world);
}
