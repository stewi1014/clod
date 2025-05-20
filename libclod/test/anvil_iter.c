#include <anvil.h>
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "nbt.h"
#include "test.h"


int main(int argc, char **argv){
    printf(TEST_DIR "world\n");

    anvil_world *world = anvil_open(TEST_DIR "world", nullptr);
    if (world == nullptr) {
        printf("open world: %s\n", anvil_strerror(anvil_errno));
        return -1;
    }

    anvil_dir *dir = anvil_open_dir(world, "region", nullptr, nullptr);
    if (dir == nullptr) {
        printf("open region dir: %s\n", anvil_strerror(anvil_errno));
        anvil_close(world);
        return -1;
    }

    anvil_iter *iter = anvil_open_iter(dir);
    if (iter == nullptr) {
        printf("open region dir: %s\n", anvil_strerror(anvil_errno));
        anvil_close_dir(dir);
        anvil_close(world);
        return -1;
    }

    struct anvil_entry ent;
    char *buffer = malloc(1 << 14);
    if (!buffer) return -1;
    size_t buffer_cap = 1 << 14;
    while (anvil_iter_next(iter, &ent) == ANVIL_OK) {
        printf(
            "region (%"PRId64", %"PRId64"): %s\n",
            ent.region_x,
            ent.region_z,
            ent.filename
        );

        anvil_file *file = anvil_iter_open_file(iter);

        for (int64_t x = 0; x < 32; x++) for (int64_t z = 0; z < 32; z++) {

            printf("chunk (%"PRId64", %"PRId64") ", x, z);

        read_again:
            size_t chunk_size;
            const anvil_result res = anvil_read(
                buffer,
                buffer_cap,
                &chunk_size,
                x, z,
                file
            );

            if (res == ANVIL_TOO_SMALL) {
                char *new = realloc(buffer, chunk_size);
                if (!new) return -1;

                buffer = new;
                buffer_cap = chunk_size;
                goto read_again;
            }

            if (res != ANVIL_OK) {
                printf("corrupted\n");
                continue;
            }

            char *status = nullptr; size_t status_size = 0;
            nbt_named(buffer, buffer + chunk_size,
                "Status", strlen("Status"), NBT_STRING, &status, &status_size,
                nullptr
            );

            if (status == nullptr) {
                printf("unable to get status\n");
            } else {
                printf("status: %.*s\n", (int)status_size, status);
            }

        }

        anvil_close_file(file);
    }

    free(buffer);
    anvil_close_iter(iter);
    anvil_close_dir(dir);
    anvil_close(world);

    return 0;
}
