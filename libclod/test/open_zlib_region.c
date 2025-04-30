#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <anvil.h>
#include <nbt.h>

#define CHUNKS 32

int main(int argc, char **argv) {
    FILE *f = fopen("r.-1.1.mca", "rb");
    if (f == NULL) {
        printf("fopen: %s\n", strerror(errno));
        return -1;
    }

    struct anvil_region region;
    region.region_x = -1;
    region.region_z = 1;
    region.data_size = 9773056;
    region.data = malloc(region.data_size);

    struct timespec read_start, read_end;
    timespec_get(&read_start, TIME_UTC);
    size_t read = fread(region.data, region.data_size, 1, f);
    timespec_get(&read_end, TIME_UTC);
    assert(read == 1);
    fclose(f);

    long read_ns = 
        (read_end.tv_sec * 1000000000L + read_end.tv_nsec) -
        (read_start.tv_sec * 1000000000L + read_start.tv_nsec);

    struct anvil_chunk_ctx *chunk_ctx = anvil_chunk_ctx_alloc(NULL);
    struct anvil_chunk chunk;

    struct timespec decompress_start, decompress_end;
    struct timespec nbt_visit_start, nbt_visit_end;
    long decompress_ns = 0, nbt_visit_ns = 0;
    size_t size_total = 0;

    for (int x = 0; x < CHUNKS; x++) for (int z = 0; z < CHUNKS; z++) {
        timespec_get(&decompress_start, TIME_UTC);
        chunk = anvil_chunk_decompress(chunk_ctx, &region, x, z);
        char *end = chunk.data + chunk.data_size;
        timespec_get(&decompress_end, TIME_UTC);

        timespec_get(&nbt_visit_start, TIME_UTC);

        char *tag = chunk.data;
        nbt_compound_foreach(nbt_payload(chunk.data, NBT_COMPOUND), end, tag, {
            //printf("%.*s(%s)\n", nbt_name_size(tag), nbt_name(tag), nbt_type_as_string(nbt_type(tag)));
        });
        
        timespec_get(&nbt_visit_end, TIME_UTC);

        assert(tag - chunk.data == chunk.data_size);
        size_total += tag - chunk.data;

        decompress_ns += 
            (decompress_end.tv_sec * 1000000000L + decompress_end.tv_nsec) -
            (decompress_start.tv_sec * 1000000000L + decompress_start.tv_nsec);

        nbt_visit_ns += 
            (nbt_visit_end.tv_sec * 1000000000L + nbt_visit_end.tv_nsec) -
            (nbt_visit_start.tv_sec * 1000000000L + nbt_visit_start.tv_nsec);
    }

    printf(
        "open_region: %0.3fms file read, %0.3fms decompressing, %0.3fms visiting NBT data\n",
        (double)read_ns/1000000.0,
        (double)decompress_ns/1000000.0,
        (double)nbt_visit_ns/1000000.0
    );

    printf(
        "open_region: %0.3fms/chunk decompressing, %0.3fms/chunk visiting NBT data\n",
        (double)decompress_ns/(1000000.0*CHUNKS*CHUNKS),
        (double)nbt_visit_ns/(1000000.0*CHUNKS*CHUNKS)
    );

    printf(
        "%0.1fMiB/s\n",
        ((double)size_total / (1024 * 1024)) / ((double)(decompress_ns + nbt_visit_ns + read_ns) / 1000000000.0)
    );

    anvil_chunk_ctx_free(chunk_ctx);

    return 0;
}
