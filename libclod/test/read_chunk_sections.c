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
            
            struct anvil_section_iter section;
            if (anvil_section_iter_init(&section, chunk)) {
                printf("ewwor\n");
                __builtin_trap();
            }

            while (!anvil_section_iter_next(&section)) {
                printf(
                    "region (%d, %d), section(%d, %d, %d), %p %p %p %p %p %p\n", 
                    region.region_x, region.region_z,
                    chunk.chunk_x, section.section_y, chunk.chunk_z,
                    section.block_state_palette,
                    section.block_state_array,
                    section.biome_palette,
                    section.biome_array,
                    section.block_light,
                    section.sky_light
                );
            }
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
