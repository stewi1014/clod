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
