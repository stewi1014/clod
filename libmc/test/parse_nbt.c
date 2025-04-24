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
    fread(chunk_data, 60628, 1, f);
    fclose(f);

    assert(nbt_step(chunk_data) == chunk_data+60628);

    char *status = nbt_payload(chunk_data, NBT_COMPOUND);
    while(strncmp(nbt_name(status), "Status", nbt_name_size(status)))
        status = nbt_step(status);

    printf(
            "Status: %.*s\n", 
            nbt_string_size(nbt_payload(status, NBT_STRING)),
            nbt_string(nbt_payload(status, NBT_STRING))
    );

    for (char *tag = nbt_payload(chunk_data, NBT_COMPOUND); nbt_type(tag) != NBT_END; tag = nbt_step(tag)) {
        printf("%.*s\n", nbt_name_size(tag), nbt_name(tag));
    }

    return 0;
}
