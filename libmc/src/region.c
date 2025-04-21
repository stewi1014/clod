#include <stdio.h>
#include <libgen.h>
#include <errno.h>

#include "mc.h"
#include "file.h"

struct MC_RegionFile {
    struct file_buffer file;
};

struct MC_RegionFile *MC_open_region_file(char *path) {
    region_file->file = file_open(path);
    if (region_file->file.data == NULL) return -1;


}

void MC_close_region_file(struct MC_RegionFile) {

}