#include <anvil.h>
#include <stdlib.h>

void anvil_opts_default(anvil_opts *opts) {
    opts->region_extension = "mca";
    opts->chunk_extension = "mcc";
    opts->coord_count = 2;
    opts->region_extent = 32;
    opts->section_size = 4096;
    opts->malloc = malloc;
    opts->calloc = calloc;
    opts->realloc = realloc;
    opts->free = free;
}
