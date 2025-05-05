#include <stddef.h>

#include "dh.h"

struct dh_lod_writer {

};

struct dh_lod_writer *dh_lod_writer_start(struct dh_lod *) {
    return NULL;
}

size_t dh_lod_writer_write(char *dest, size_t dest_size) {
    return 0;
}

void dh_lod_writer_stop(struct dh_lod_writer *) {

}
