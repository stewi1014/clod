/**
 * region.h
 * 
 * Provides a method for reading decompressed chunk data from region files.
 */

#pragma once

/**
 * World Reading
 */

struct region_reader;
struct region_reader *region_reader_alloc();
void region_reader_free(struct region_reader *);

/** 
 * finds the data for the given chunk,
 * decompresses it,
 * and returns a pointer to the start of the NBT data.
 * 
 * if region is NULL it returns NULL.
 * if reader is NULL it creates a temporary one that is freed before returning.
 * 
 * region_chunk_reader holds allocated memory that can be reused between chunk reads.
 */
char *region_read_chunk(
    struct region_reader *reader, 
    char *region, 
    int chunk_x, 
    int chunk_z
);
