# libclod

man, pages are missing.

## Structure

- `#include <nbt.h>` for parsing NBT data.
- `#include <world.h>` for world reading methods.
- `#include <dh.h>` tor methods for dealing with DistantHorizons LODs and DBs.

## Non-Goals

It can be easy to get carried away on flights of fancy and waste time better spent elsewhere on nonsense
that slows both you and the program down. The non-goals of this project are notes to myself that aim to quell this problem.

- **multithreading**: if the library is so slow it requires multithreading then it has become too complex.
- **caches**: websites need cache because users are a source of random behaviour - if code is generating random behaviour it needs fixing.
- **IO abstraction**: wrapping IO methods implies a better understanding of the IO device than the user - which is arrogant and rarely correct.
- **pools**: correct resource management should be implemented instead of connection pools, buffer pools or any other kind of pool.
- **platform independence**: if your char is 9-bits long, pointers are tuples and numbers are middle-endian - I'm sorry, but no.
- **fixing data**: garbage in -> garbage out. data parsing should never segfault on bad data however. garbage in ->X segfault out.

## Headers

### [nbt.h](./include/nbt.h)

NBT parsing library that does not offer any intermediate representation of NBT data.

Methods operate directly on the serialised data, which makes it fast as all get-out for parsing and making changes to serialised data.
However some use cases do not need changes to be serialised,
and can benifit from storing changes in an intermediate data format - this library is not for those use cases.

This approach also comes with an ergonomic downside;
Nuances of the NBT format are not abstracted away by this library, and using it can be verbose.
So, if you're going to use it, make sure you're aware of how the NBT format works and
note that even if you know how the NBT format works, this library will still be a pain to use.

#### Examples

##### Finding a specific tag with comprehensive error checking

```C
size_t decompressed_size;
char *chunk_data = region_read_chunk(reader, region_file, x, z, &decompressed_size);
char *end = chunk_data + decompressed_size;

char *status = nbt_named(chunk_data, end, "Status")
if (status == NULL) {
    printf("corrupted NBT data.\n");
} else if (nbt_type(status) == NBT_END) {
    printf("no tag with name 'Status' found.\n");
} else if (nbt_type(status) != NBT_STRING) {
    printf(
        "tag with name 'Status' exists, but is of type %s. Expected NBT_STRING\n", 
        nbt_type_as_string(nbt_type(status))
    );
} else {
    printf(
        "chunk status: %.*s\n", 
        nbt_string_size(nbt_payload(status, NBT_STRING)),
        nbt_string(nbt_payload(status, NBT_STRING))
    );
}

```

##### Iterating over all tags in compound with "couldn't care less" error checking

```C

size_t decompressed_size;
char *chunk_data = region_read_chunk(reader, region_file, x, z, &decompressed_size);
char *end = chunk_data + decompressed_size;

char *tag;
nbt_compound_foreach(nbt_payload(chunk_data, NBT_COMPOUND), end, tag, {
    printf("%.*s(%s)\n", nbt_name_size(tag), nbt_name(tag), nbt_type_as_string(nbt_type(tag)));
});

```

##### Iterating over a specific nested tag with "couldn't care less" error checking

```C

size_t decompressed_size;
char *chunk_data = region_read_chunk(reader, region_file, x, z, &decompressed_size);
char *end = chunk_data + decompressed_size;

char *section, *child;
nbt_list_foreach(nbt_payload(nbt_named(chunk_data, end, "sections"), NBT_LIST), end, section, 
    nbt_compound_foreach(nbt_payload(section, NBT_COMPOUND), end, child, {
        printf("%.*s(%s)\n", nbt_name_size(child), nbt_name(child), nbt_type_as_string(nbt_type(child)));
    })
);

```

### [world.h](./include/world.h)

### [dh.h](./include/dh.h)

## Dependencies

- libdeflate
- liblz4
- libzstd
- liblzma
- sqlite3
- libpq

# References

[https://github.com/Celisium/libnbt/](https://github.com/Celisium/libnbt/)
[https://minecraft.wiki/w/NBT_format](https://minecraft.wiki/w/NBT_format)
