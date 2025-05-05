#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <anvil.h>
#include <nbt.h>

int main(int argc, char **argv) {
    struct anvil_world *world = anvil_open("world");
    if (world == NULL) {
        printf("open world: %s\n", strerror(errno));
        return -1;
    }

    struct timespec start, end;
    struct timespec decompress_start, decompress_end;
    struct timespec nbt_visit_start, nbt_visit_end;
    long decompress_ns = 0, nbt_visit_ns = 0, total_ns;
    size_t size_total = 0, chunk_total = 0;
    timespec_get(&start, TIME_UTC);
    
    struct anvil_chunk_ctx *chunk_ctx = anvil_chunk_ctx_alloc(NULL);
    struct anvil_region_iter *iter = anvil_region_iter_new("region", world);
    struct anvil_region region;
    struct anvil_sections sections = ANVIL_SECTIONS_CLEAR;

    int error;
    while (!(error = anvil_region_iter_next(&region, iter))) {
        //printf("(%d, %d) ", region.region_x, region.region_z);

        struct anvil_chunk chunk;
    
        for (int x = 0; x < 32; x++) for (int z = 0; z < 32; z++) {
            timespec_get(&decompress_start, TIME_UTC);

                chunk = anvil_chunk_decompress(chunk_ctx, &region, x, z);
                if (chunk.data_size == 0) continue;

            timespec_get(&decompress_end, TIME_UTC);
            
            
            timespec_get(&nbt_visit_start, TIME_UTC);

                if (anvil_parse_sections(&sections, chunk)) {
                    printf("ewwor\n");
                    __builtin_trap();
                }

                for (int i = 0; i < sections.len; i++) {
                    printf(
                        "region (%d, %d), section(%d, %d, %d), %p %p %p %p %p %p\n", 
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
            
            timespec_get(&nbt_visit_end, TIME_UTC);
    
            chunk_total++;
            size_total += chunk.data_size;
            decompress_ns += (decompress_end.tv_sec * 1000000000L + decompress_end.tv_nsec) - (decompress_start.tv_sec * 1000000000L + decompress_start.tv_nsec);
            nbt_visit_ns += (nbt_visit_end.tv_sec * 1000000000L + nbt_visit_end.tv_nsec) - (nbt_visit_start.tv_sec * 1000000000L + nbt_visit_start.tv_nsec);
        }        
    }
    if (error < 0) {
        printf("%s\n", strerror(errno));
        return -1;
    }

    timespec_get(&end, TIME_UTC);

    total_ns = 
        (end.tv_sec * 1000000000L + end.tv_nsec) -
        (start.tv_sec * 1000000000L + start.tv_nsec);

    printf(
        "injested %0.1fMB of chunk data in %0.3fms: %0.3fMB/s\n",
        (double)size_total / 1000000.0,
        (double)total_ns / 1000000.0,
        (double)size_total * 1000 / total_ns
    );

    printf(
        "open_world: %0.3fms total decompressing, %0.3fms total visiting NBT data, %0.3fms total everything else (opening files)\n",
        (double)decompress_ns / 1000000.0,
        (double)nbt_visit_ns / 1000000.0,
        (double)(total_ns - decompress_ns - nbt_visit_ns) / 1000000.0
    );

    printf(
        "open_world: %0.3fms/chunk decompressing, %0.3fms/chunk visiting NBT data, %0.3fms/chunk everything else (opening files)\n",
        (double)decompress_ns / (1000000.0 * chunk_total),
        (double)nbt_visit_ns / (1000000.0 * chunk_total),
        (double)(total_ns - decompress_ns - nbt_visit_ns) / (1000000.0 * chunk_total)
    );
    
    anvil_chunk_ctx_free(chunk_ctx);
    anvil_region_iter_free(iter);
    anvil_sections_free(&sections);
    anvil_close(world);

    return 0;
}
