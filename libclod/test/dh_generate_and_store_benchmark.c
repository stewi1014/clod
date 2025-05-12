#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include <anvil.h>
#include <dh.h>

#define ANVIL_WORLD "/home/stewi/Dev/gitlab.com/distant-horizons-team/distant-horizons/run/client/saves/New World"
#define DH_DATABASE "/home/stewi/Dev/gitlab.com/distant-horizons-team/distant-horizons/run/client/saves/New World/data/DistantHorizons.sqlite"

int main(int argc, char **argv) {
    struct anvil_world *world = anvil_open(ANVIL_WORLD);
    if (world == NULL) {
        printf("open world: %s\n", strerror(errno));
        return -1;
    }

    remove(DH_DATABASE);
    struct dh_db *db = dh_db_open(DH_DATABASE);
    assert(db != NULL);

    struct timespec start, end;
    struct timespec read_start, read_end;
    struct timespec decompress_start, decompress_end;
    struct timespec lod_start, lod_end;
    struct timespec compress_start, compress_end;
    struct timespec store_start, store_end;

    long read_ns = 0, decompress_ns = 0, lod_gen_ns = 0, compress_ns = 0, store_ns = 0, total_ns;
    size_t read_nbytes = 0, decompress_nbytes = 0, lod_allocated_nbytes = 0, lod_nbytes = 0, compress_nbytes = 0, compressed_nbytes = 0, store_nbytes = 0;
    size_t num_regions = 0, num_chunks = 0, num_lods = 0, num_store = 0;
    timespec_get(&start, TIME_UTC);
    
    struct anvil_chunk_ctx *chunk_ctx[16];
    for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++) {
        chunk_ctx[xi * 4 + zi] = anvil_chunk_ctx_alloc(NULL);
    }

    struct anvil_region_iter *iter = anvil_region_iter_new("region", world);
    struct anvil_region region;
    struct anvil_chunk chunks[16];
    struct dh_lod lod = DH_LOD_CLEAR;
    dh_result result;

    int error;
    timespec_get(&read_start, TIME_UTC);
    while (!(error = anvil_region_iter_next(&region, iter))) {
        //printf("(%d, %d) ", region.region_x, region.region_z);

        timespec_get(&read_end, TIME_UTC);

        num_regions++;
        read_nbytes += region.data_size;
        read_ns += 
            (read_end.tv_sec * 1000000000L + read_end.tv_nsec) - 
            (read_start.tv_sec * 1000000000L + read_start.tv_nsec);

        for (int x = 0; x < 32; x += 4) for (int z = 0; z < 32; z += 4) {
            // decompress chunk data
            timespec_get(&decompress_start, TIME_UTC);
            for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++) {
                chunks[xi * 4 + zi] = anvil_chunk_decompress(chunk_ctx[xi * 4 + zi], &region, x + xi, z + zi);
                decompress_nbytes += chunks[xi * 4 + zi].data_size;
                num_chunks++;
            }
            timespec_get(&decompress_end, TIME_UTC);

            decompress_ns += 
                (decompress_end.tv_sec * 1000000000L + decompress_end.tv_nsec) - 
                (decompress_start.tv_sec * 1000000000L + decompress_start.tv_nsec);
            
            // generate LOD
            timespec_get(&lod_start, TIME_UTC);
                result = dh_from_chunks(chunks, &lod);
            timespec_get(&lod_end, TIME_UTC);

            assert(result == DH_OK);
            lod_nbytes += lod.lod_len;
            lod_allocated_nbytes += lod.lod_cap;
            num_lods++;
            lod_gen_ns += 
                (lod_end.tv_sec * 1000000000L + lod_end.tv_nsec) -
                (lod_start.tv_sec * 1000000000L + lod_start.tv_nsec);

            if (!lod.has_data) continue;

            // compress LOD
            timespec_get(&compress_start, TIME_UTC);
                compress_nbytes += lod.lod_len;
                result = dh_compress(&lod, DH_DATA_COMPRESSION_LZ4, 0.75);
                compressed_nbytes += lod.lod_len;
            timespec_get(&compress_end, TIME_UTC);

            assert(result == DH_OK);
            compress_ns +=
                (compress_end.tv_sec * 1000000000L + compress_end.tv_nsec) -
                (compress_start.tv_sec * 1000000000L + compress_start.tv_nsec);

            // store LOD
            timespec_get(&store_start, TIME_UTC);
                result = dh_db_store(db, &lod);
                assert(result == 0);
            
                store_nbytes += lod.lod_len;
                num_store++;
            timespec_get(&store_end, TIME_UTC);

            store_ns += 
                (store_end.tv_sec * 1000000000L + store_end.tv_nsec) -
                (store_start.tv_sec * 1000000000L + store_start.tv_nsec);
        }
        
        timespec_get(&read_start, TIME_UTC);
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
        "read         %8.1fMB of world data from %7ld regions in %9.1fms (%6.3fms/region), %7.3fMB/s overall, %9.3fMB/s while active, %8.3f regions/second overall\n",
        (double)read_nbytes / 1000000.0,
        num_regions,
        (double)read_ns / 1000000.0,
        (double)read_ns / (1000000.0 * num_regions),
        (double)read_nbytes * 1000 / total_ns,
        (double)read_nbytes * 1000 / read_ns,
        ((double)num_regions * 1000000000.0) / total_ns
    );

    printf(
        "decompressed %8.1fMB of chunk data from %7ld chunks in  %9.1fms (%6.3fms/chunk),  %7.3fMB/s overall, %9.3fMB/s while active, %8.3f chunks/second overall\n",
        (double)decompress_nbytes / 1000000.0,
        num_chunks,
        (double)decompress_ns / 1000000.0,
        (double)decompress_ns / (1000000.0 * num_chunks),
        (double)decompress_nbytes * 1000 / total_ns,
        (double)decompress_nbytes * 1000 / decompress_ns,
        ((double)num_chunks * 1000000000.0) / total_ns
    );

    printf(
        "generated    %8.1fMB of LOD data in     %7ld LODs in    %9.1fms (%6.3fms/LOD),    %7.3fMB/s overall, %9.3fMB/s while active, %8.3f LODs/second overall\n",
        (double)lod_nbytes / 1000000.0,
        num_lods,
        (double)lod_gen_ns / 1000000.0,
        (double)lod_gen_ns / (1000000.0 * num_lods),
        (double)lod_nbytes * 1000 / total_ns,
        (double)lod_nbytes * 1000 / lod_gen_ns,
        ((double)num_lods * 1000000000.0) / total_ns
    );

    printf(
        "compressed   %8.1fMB of LOD data in     %7ld LODs in    %9.1fms (%6.3fms/LOD),    %7.3fMB/s overall, %9.3fMB/s while active, %8.3f LODs/second overall\n",
        (double)compress_nbytes / 1000000.0,
        num_lods,
        (double)compress_ns / 1000000.0,
        (double)compress_ns / (1000000.0 * num_lods),
        (double)compress_nbytes * 1000 / total_ns,
        (double)compress_nbytes * 1000 / compress_ns,
        ((double)num_lods * 1000000000.0) / total_ns
    );

    printf(
        "stored       %8.1fMB of LOD data from   %7ld LODs in    %9.1fms (%6.3fms/LOD),    %7.3fMB/s overall, %9.3fMB/s while active, %8.3f LODs/second overall\n",
        (double)store_nbytes / 1000000.0,
        num_store,
        (double)store_ns / 1000000.0,
        (double)store_ns / (1000000.0 * num_store),
        (double)store_nbytes * 1000 / total_ns,
        (double)store_nbytes * 1000 / store_ns,
        ((double)num_store * 1000000000.0) / total_ns
    );
    
    for (int xi = 0; xi < 4; xi++) for (int zi = 0; zi < 4; zi++) {
        anvil_chunk_ctx_free(chunk_ctx[xi * 4 + zi]);
    }

    dh_db_close(db);
    dh_lod_free(&lod);
    anvil_region_iter_free(iter);
    anvil_close(world);

    return 0;
}
