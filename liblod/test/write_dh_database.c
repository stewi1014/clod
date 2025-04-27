#include <stdio.h>
#include <assert.h> 

#include <dh.h>

int main(int argc, char **argv) {
    struct dh_db *db = dh_db_open("DistantHorizons.sqlite");
    assert(db != NULL);
    
    struct dh_lod lod = {
        .detail_level = 0,
        .pos_x = 0, 
        .pos_z = 0,
        .min_y = 0,
        .data_checksum = 0,
        .data = 0,
        .column_generation_step = 0,
        .column_world_compression_mode = 0,
        .mapping = 0,
        .data_format_version = 0,
        .compression_mode = 0,
        .apply_to_parent = 0,
        .apply_to_children = 0,
        .last_modified_time = 0,
        .created_time = 0
    };

    int error = dh_db_store(db, &lod);
    assert(error == 0);

    dh_db_close(db);

    remove("DistantHorizons.sqlite");
}
