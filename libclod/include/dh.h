/**
 * dh.h
 * 
 * Methods for dealing with DH LODs.
 */

#pragma once
#include <stdbool.h>
#include <time.h>

struct dh_db;
struct dh_db *dh_db_open();
void dh_db_close(struct dh_db *);

struct dh_db_lod {
    char detail_level;
    int pos_x;
    int pos_z;
    int min_y;
    int data_checksum;
    char *data;
    char *column_generation_step;
    char *column_world_compression_mode;
    char *mapping;
    char data_format_version;
    char compression_mode;
    bool apply_to_parent;
    bool apply_to_children;
    time_t last_modified_time;
    time_t created_time;
};

int dh_db_store(struct dh_db *db, struct dh_db_lod *lod);

/**
 * 
 */
