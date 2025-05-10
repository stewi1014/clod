# libclod

Library for dealing with minecraft data - including LODs.

This library will probably never be used by anyone except myself,
and the world is probably better off for it.
Having said that, it isn't *terrible*, and offers some useful functionality.

If you're using a higher level language and need some things done *really* fast,
or you're writing C that needs to deal with minecarft data and don't feel like DIY-ing
the multi-faceted technology stack that is the minecraft save file,
I'd recomend at least using this library as a reference.

## Structure

- [`#include <anvil.h>`](./include/anvil.h) reads the anvil world format.
- [`#include <dh.h>`](./include/dh.h) deals with DistantHorizons LODs and databases.
- [`#include <lod.h>`](./include/lod.h) for LOD generating.
- [`#include <nbt.h>`](./include/nbt.h) gives you NBT parsing helpers.

## Non-Goals

It can be easy to get carried away on flights of fancy and waste time better spent elsewhere on nonsense
that slows both you and the program down. The non-goals of this project are notes to myself that aim to quell this problem.

- **multithreading**: if the library is so slow it requires multithreading then it has become too complex.
- **caches**: websites need cache because users are a source of random behaviour - if code is generating random behaviour it needs fixing.
- **IO**: hardcoding IO implementations implies a better understanding of the IO device than the user - which is arrogant and rarely correct.
- **pools**: correct resource management should be implemented instead of connection pools, buffer pools or any other kind of pool.
- **platform independence**: if your char is 9-bits long, pointers are tuples and numbers are middle-endian - I'm sorry, but no.
- **fixing data**: garbage in -> garbage out. data parsing should never segfault on bad data however. garbage in ->X segfault out.
- **bad abstractions**: e.g. 'get_chunk(x,y)' is a bad abstraction because it implies speed and simplicity - the opposite of the implementation which is complex and IO-heavy.

## Headers

### [anvil.h](./include/anvil.h)

Methods for reading the anvil world format.

#### Example: Iterating over all sections in a world

```C

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
    struct anvil_region_iter *iter = anvil_region_iter_new("region", world); // directory with region files, e.g. 'region' 'DIM-1', 'DIM1'.
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

```

### [dh.h](./include/dh.h)

Methods for dealing with DH LODs

#### Example: Generate DH LODs for entire minecraft world

```C

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

```

### [lod.h](./include/lod.h)

### [nbt.h](./include/nbt.h)

Very *very* fast NBT parsing library: ~5µs/chunk to traverse all chunk data in a world.
It does not offer any intermediate representation of NBT data.

Methods operate directly on the serialised data, which makes it fast as all get-out for parsing and making changes to serialised data.
However some use cases do not need changes to be serialised,
and can benefit from storing changes in an intermediate data format - this library is not for those use cases.
For example, changing the name of a tag 100 times (with different lengths) means all data serialised after it needs to be moved 100 times,
wheras copying the NBT data to a different location in memory allows the copy to be modified without reshuffling.

This approach also comes with an ergonomic downside;
It's quite low-level - nuances of the NBT format are not abstracted away by this library, not really.
It is cumbersome to use, and it's easy to write users of it that accidentally step through NBT data multiple times
(one of the examples below does exactly that!).
So, if you're going to use it, make sure you're aware of how the NBT format works and
note that even if you know how the NBT format works, this library will still be a pain to use.

#### Example: Get chunk status

```C

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <nbt.h>

int main(int argc, char **argv) {
    FILE *f = fopen("chunk_data.nbt", "rb");
    if (f == NULL) {
        printf("%s\n", strerror(errno));
        return -1;
    }

    char *chunk_data = malloc(60628);
    char *end = chunk_data+60628;
    assert(fread(chunk_data, 60628, 1, f) == 1);
    fclose(f);

    assert(nbt_step(chunk_data, end) == end);

    char *status = nbt_payload(chunk_data, NBT_COMPOUND, end);
    while(strncmp(nbt_name(status, end), "Status", nbt_name_size(status, end)))
        status = nbt_step(status, end);

    printf(
            "Status: %.*s\n", 
            nbt_string_size(nbt_payload(status, NBT_STRING, end)),
            nbt_string(nbt_payload(status, NBT_STRING, end))
    );

    return 0;
}

```

## Dependencies

Make sure these are findable by meson.

- libdeflate
- liblz4
- libzstd
- liblzma
- sqlite3
- libpq

# References

[https://github.com/Celisium/libnbt/](https://github.com/Celisium/libnbt/)
[https://minecraft.wiki/w/NBT_format](https://minecraft.wiki/w/NBT_format)
