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

    char *tag = NULL;
    nbt_compound_foreach(nbt_payload(chunk_data, NBT_COMPOUND, end), end, tag, {
        //printf("%.*s(%s)\n", nbt_name_size(tag), nbt_name(tag), nbt_type_as_string(nbt_type(tag)));
    });
    assert(tag == end);

    char *sections = nbt_named(nbt_payload(chunk_data, NBT_COMPOUND, end), "sections", end);
    char *section;
    nbt_list_foreach(nbt_payload(sections, NBT_LIST, end), NULL, section, {
        char *y_val = nbt_named(section, "Y", end);


        nbt_compound_foreach(section, NULL, tag, {
            printf("%.*s(%s)\n", nbt_name_size(tag, end), nbt_name(tag, end), nbt_type_as_string(nbt_type(tag, end)));
        });

    
        printf("section Y: %ld\n", nbt_autointeger(y_val, end));
    });
    assert(section == nbt_step(sections, NULL));

    return 0;
}
