/**
 * anvil.h
 * 
 * methods for dealing with the anvil world format.
 */

#pragma once
#include <stddef.h>

/** region_reader holds allocated memory that can be reused between chunk reads. */
struct region_reader;
struct region_reader *region_reader_alloc();
void region_reader_free(struct region_reader *);

/** 
 * finds the data for the given chunk,
 * decompresses it,
 * and returns a pointer to the start of the NBT data.
 * 
 * the returned buffer is owned by the region_reader,
 * and is only valid until the next call.
 * 
 * if reader is NULL it returns NULL.
 * if region is NULL it returns NULL.
 */
char *region_read_chunk(
    struct region_reader *reader, 
    char *region_file, 
    int chunk_x, 
    int chunk_z,
    size_t *size
);
