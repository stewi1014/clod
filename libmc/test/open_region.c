#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <region.h>
#include <nbt.h>

int main(int argc, char **argv) {
    FILE *f = fopen("r.-1.1.mca", "rb");
    if (f == NULL) {
        printf("%s\n", strerror(errno));
        return -1;
    }

    char *region = malloc(9773056); //uuh
    struct timespec read_start, read_end;
    timespec_get(&read_start, TIME_UTC);
    size_t read = fread(region, 9773056, 1, f);
    timespec_get(&read_end, TIME_UTC);
    assert(read == 1);
    fclose(f);

    long read_ns = 
        (read_end.tv_sec * 1000000000L + read_end.tv_nsec) -
        (read_start.tv_sec * 1000000000L + read_start.tv_nsec);

    struct region_reader *reader = region_reader_alloc();
    struct timespec decompress_start, decompress_end;
    struct timespec nbt_visit_start, nbt_visit_end;
    long decompress_ns = 0, nbt_visit_ns = 0;
    size_t size_total = 0;

    for (int x = 0; x < 32; x++) for (int z = 0; z < 32; z++) {
        timespec_get(&decompress_start, TIME_UTC);
        char *chunk_data = region_read_chunk(reader, region, x, z);
        timespec_get(&decompress_end, TIME_UTC);

        timespec_get(&nbt_visit_start, TIME_UTC);
        //char *nbt_end = nbt_step(chunk_data);

        char *status = nbt_payload(chunk_data, NBT_COMPOUND);
        while(strncmp(nbt_name(status), "Status", nbt_name_size(status)))
            status = nbt_step(status);
    
        timespec_get(&nbt_visit_end, TIME_UTC);

        //size_total += nbt_end - chunk_data;

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
        (double)decompress_ns/(1000000.0*32*32),
        (double)nbt_visit_ns/(1000000.0*32*32)
    );

    printf(
        "%0.1fMiB/s\n",
        ((double)size_total / (1024 * 1024)) / ((double)(decompress_ns + nbt_visit_ns) / 1000000000.0)
    );

    region_reader_free(reader);

    return 0;
}
