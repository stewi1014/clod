#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
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
    struct anvil_sections sections = ANVIL_SECTIONS_CLEAR;

    int error;
    while (!(error = anvil_region_iter_next(&region, iter))) {
        struct anvil_chunk chunk;
        for (int x = 0; x < 32; x++) for (int z = 0; z < 32; z++) {
            chunk = anvil_chunk_decompress(chunk_ctx, &region, x, z);
            if (chunk.data_size == 0) continue; // files often have empty chunks.
            
            if (anvil_parse_sections(&sections, chunk)) {
                printf("ewwor\n");
                __builtin_trap();
            }

            for (int64_t i = 0; i < sections.len; i++) {
                printf(
                    "region (%ld, %ld), section(%ld, %ld, %ld), %p %p %p %p %p %p\n", 
                    region.region_x, region.region_z,
                    chunk.chunk_x, i + sections.min_y, chunk.chunk_z,
                    sections.section[i].block_state_palette,
                    sections.section[i].block_state_indicies,
                    sections.section[i].biome_palette,
                    sections.section[i].biome_indicies,
                    sections.section[i].block_light,
                    sections.section[i].sky_light
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
    anvil_sections_free(&sections);
    anvil_close(world);
}
