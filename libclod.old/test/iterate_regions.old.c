#include <anvil.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv) {
    struct anvil_world *world;
    struct anvil_dir *dir;
    struct anvil_iter *iter;

    anvil_result res = anvil_world_open(&world, "world", nullptr);
    assert(res == ANVIL_OK);

    res = anvil_world_open_dir(&dir, world, "region", nullptr, nullptr);
    assert(res == ANVIL_OK);

    res = anvil_iter_open(&iter, dir);
    assert(res == ANVIL_OK);

    struct anvil_entry entry;
    while ((res = anvil_iter_next(&entry, iter)) == ANVIL_OK) {
        printf("region file %s\n", entry.filename);
    }
    assert(res == ANVIL_DONE);

    anvil_iter_close(iter);
    anvil_dir_close(dir);
    anvil_world_close(world);
    return 0;
}
