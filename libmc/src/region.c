#include "mc.h"

/*

struct MC_ChunkLocation {
    char sector_count: 8;
    unsigned int sector_offset: 24;
};

struct MC_RegionFile {
    struct file_buffer file;
    struct {
        char *data;
        unsigned char sectors;
        unsigned int last_modified;
    } chunk[1024];
};

struct MC_RegionFile *MC_open_region_file(char *path) {
    struct file_buffer file = file_open(path);
    if (file.data == NULL) return NULL;

    struct MC_RegionFile *region_file = malloc(sizeof(struct MC_RegionFile));
    region_file->file = file;

    char *loc = region_file->file.data;
    for (int i = 0; i < 1024; i++, loc+=4) {
        int offset = (loc[0]<<16) + (loc[1]<<8) + loc[2];
        int sectors = loc[3];

        if (offset == 0 || sectors == 0) {
            region_file->chunk[i].data = NULL;
            region_file->chunk[i].sectors = 0;
        } else {
            region_file->chunk[i].data = region_file->file.data + offset * 4096;
            region_file->chunk[i].sectors = sectors;
        }
    }

    for (int i = 0; i < 1024; i++, loc+=4) {
        int last_modified = (loc[0]<<24) + (loc[1]<<16) + (loc[2]<<8) + loc[3];
        region_file->chunk[i].last_modified = last_modified;
    }

    return region_file;
}

void MC_close_region_file(struct MC_RegionFile *region_file) {
    struct file_buffer file = region_file->file;
    free(region_file);
    file_close(file);
}

time_t MC_region_file_mtime(struct MC_RegionFile *region_file) {
    return region_file->file.last_modified;
}

time_t MC_region_file_chunk_mtime(struct MC_RegionFile *region_file, int chunk_x, int chunk_z) {
    return region_file->chunk[(chunk_x&31) + ((chunk_z&31) * 32)].last_modified;
}

struct MC_ChunkData {
    char *nbt_data;
};

struct MC_ChunkData *MC_chunk_data_open(struct MC_RegionFile *region_file, int chunk_x, int chunk_z) {
    char *data = region_file->chunk[(chunk_x&31) + ((chunk_z&31) * 32)].data;
    if (data == NULL) return NULL;
    
    unsigned int size = (data[0] << 24) + (data[1]<<16) + (data[2]<<8) + (data[3]) - 1; // -1 because size includes compression_type byte
    char compression_type = data[4];
    data = &data[5]; // start of compressed data

    switch (compression_type) {
    case 1: // GZip (unused in practice)
        return NULL;
    case 2: // Zlib
        return NULL;
    case 3: // Uncompressed
        return NULL;
    case 4: // LZ4
        return NULL;
    default:
        return NULL;
    }
}

void MC_chunk_data_close(struct MC_ChunkData *chunk_data) {

}

*/