# libmc

A library for dealing with minecraft data.

- `#include <nbt.h>` for parsing NBT data.
- `#include <region.h>` for region file parsing.

## [nbt.h](./include/nbt.h)

Contains tools helpful for parsing NBT data.
Notably it doesn't include any intermediary data structure that NBT data is parsed into;
it doesn't attempt to abstract away the parsing proccess, not really.
Instead, it's designed to provide tools for making concice parsers.

### Examples

#### Finding a specific tag

```C
char *chunk_data = region_read_chunk(...);

char *status = nbt_payload(chunk_data, NBT_COMPOUND);
while(strncmp(nbt_name(status), "Status", nbt_name_size(status)))
    status = nbt_step(status);

printf(
    "chunk status: %.*s\n", 
    nbt_string_size(nbt_payload(status, NBT_STRING)),
    nbt_string(nbt_payload(status, NBT_STRING))
);

```

#### Iterating over all tags in compound

```C

for (char *tag = nbt_payload(compound_tag, NBT_COMPOUND); nbt_type(tag) != NBT_END; tag = nbt_step(tag)) {
    printf("%.*s\n", nbt_name_size(tag), nbt_name(tag));
}

```

#### Iterating over all paylods in a list

```C

char *elem;
for (int i = 0; elem = nbt_list_iter(list_payload, elem, i, NBT_STRING); i++) {
    printf("index %d, string value \"%.*s\"\n", nbt_string_size(elem), nbt_string(elem));
}

```

## [region.h](./include/region.h)

Provides a method for reading decompressed chunk data from region files.

```C
char *region_file_data = my_file_read_func("r.-1.1.mca");

struct region_reader *reader = region_reader_alloc();
for (int x = 0; x < 32; x++) for (int z = 0; z < 32; z++) {
    char *chunk_data = region_read_chunk(reader, region, x, z);

    char *status = nbt_payload(chunk_data, NBT_COMPOUND);
    while(strncmp(nbt_name(status), "Status", nbt_name_size(status)))
        status = nbt_step(status);

    printf(
        "chunk status: %.*s\n", 
        nbt_string_size(nbt_payload(status, NBT_STRING)),
        nbt_string(nbt_payload(status, NBT_STRING))
    );
}
region_reader_free(reader);

```

## References

[https://github.com/Celisium/libnbt/](https://github.com/Celisium/libnbt/)
[https://minecraft.wiki/w/NBT_format](https://minecraft.wiki/w/NBT_format)
