#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include <anvil.h>
#include <dh.h>

int main(int argc, char **argv) {
    struct anvil_world *world = anvil_open("world");
    if (world == NULL) {
        printf("open world: %s\n", strerror(errno));
        return -1;
    }

    struct timespec start, end;
    struct timespec decompress_start, decompress_end;
    struct timespec lod_start, lod_end;
    long decompress_ns = 0, nbt_visit_ns = 0, total_ns;
    size_t size_total = 0, chunk_total = 0;
    timespec_get(&start, TIME_UTC);
    
    struct anvil_chunk_ctx *chunk_ctx[4][4];
    for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++) {
        chunk_ctx[xi][zi] = anvil_chunk_ctx_alloc(NULL);
    }

    struct anvil_region_iter *iter = anvil_region_iter_new("region", world);
    struct anvil_region region;
    struct anvil_chunk chunks[4][4];
    struct dh_lod *lod = NULL;

    struct dh_lod_stats stats = DH_LOD_STATS_CLEAR;

    int error;
    while (!(error = anvil_region_iter_next(&region, iter))) {
        //printf("(%d, %d) ", region.region_x, region.region_z);

        for (int x = 0; x < 32; x += 4) for (int z = 0; z < 32; z += 4) {
            timespec_get(&decompress_start, TIME_UTC);

            for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++) {
                chunks[xi][zi] = anvil_chunk_decompress(chunk_ctx[xi][zi], &region, x + xi, z + zi);
                size_total += chunks[xi][zi].data_size;
                chunk_total++;
            }

            timespec_get(&decompress_end, TIME_UTC);
            
            
            timespec_get(&lod_start, TIME_UTC);

                int result = dh_generate_from_chunks(&lod, chunks);
                assert(result == 0);
            
            timespec_get(&lod_end, TIME_UTC);

            dh_lod_get_stats(&stats, lod);
    
            decompress_ns += (decompress_end.tv_sec * 1000000000L + decompress_end.tv_nsec) - (decompress_start.tv_sec * 1000000000L + decompress_start.tv_nsec);
            nbt_visit_ns += (lod_end.tv_sec * 1000000000L + lod_end.tv_nsec) - (lod_start.tv_sec * 1000000000L + lod_start.tv_nsec);
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
        "injested %ld chunks, %0.1fMB of chunk data in %0.3fms: %0.3fMB/s, %0.3f chunks/second\n",
        chunk_total,
        (double)size_total / 1000000.0,
        (double)total_ns / 1000000.0,
        (double)size_total * 1000 / total_ns,
        ((double)chunk_total * 1000000000.0) / total_ns
    );

    printf(
        "generate_dh_lods: %0.3fms total decompressing, %0.3fms total generating LODs, %0.3fms total other (opening files)\n",
        (double)decompress_ns / 1000000.0,
        (double)nbt_visit_ns / 1000000.0,
        (double)(total_ns - decompress_ns - nbt_visit_ns) / 1000000.0
    );

    printf(
        "generate_dh_lods: %0.3fms/chunk decompressing, %0.3fms/chunk generating LODs, %0.3fms/chunk everything else (opening files)\n",
        (double)decompress_ns / (1000000.0 * chunk_total),
        (double)nbt_visit_ns / (1000000.0 * chunk_total),
        (double)(total_ns - decompress_ns - nbt_visit_ns) / (1000000.0 * chunk_total)
    );

    printf(
        "LODs averaged %ldKiB in size, %ldKiB used, %ldKiB unused, %ldKiB metadata, %0.0f%% memory efficiency\n",
        ((stats.mem_metadata + stats.mem_unused + stats.mem_used) / stats.num_lods) >> 10,
        (stats.mem_used / stats.num_lods) >> 10,
        (stats.mem_unused / stats.num_lods) >> 10,
        (stats.mem_metadata / stats.num_lods) >> 10,
        (double)stats.mem_used * 100 / (stats.mem_unused + stats.mem_used)
    );
    
    for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++) {
        anvil_chunk_ctx_free(chunk_ctx[xi][zi]);
    }

    dh_lod_free(lod);
    anvil_region_iter_free(iter);
    anvil_close(world);

    return 0;
}
