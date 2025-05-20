#include "anvil.h"

struct anvil_file {
    coord_layout_t layout;
    int64_t *region;
};

struct anvil_file *anvil_open_file(
    coord_layout_t layout,
    int64_t *region
);

bool anvil_close_file(struct anvil_file *file);
