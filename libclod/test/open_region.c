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
    size_t decompressed_size;

    for (int x = 0; x < CHUNKS; x++) for (int z = 0; z < CHUNKS; z++) {
        timespec_get(&decompress_start, TIME_UTC);
        char *chunk_data = region_read_chunk(reader, region, x, z, &decompressed_size);
        char *end = chunk_data + decompressed_size;
        timespec_get(&decompress_end, TIME_UTC);

        timespec_get(&nbt_visit_start, TIME_UTC);

        char *tag = NULL;
        nbt_compound_foreach(nbt_payload(chunk_data, NBT_COMPOUND), end, tag, {
            //printf("%.*s(%s)\n", nbt_name_size(tag), nbt_name(tag), nbt_type_as_string(nbt_type(tag)));
        });
        
        timespec_get(&nbt_visit_end, TIME_UTC);

        assert(tag - chunk_data == decompressed_size);
        size_total += tag - chunk_data;

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

    region_reader_free(reader);

    return 0;
}
