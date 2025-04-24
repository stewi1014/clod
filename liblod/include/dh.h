/**
 * dh.h
 * 
 * Methods for dealing with DH LODs.
 */

#pragma once

struct dh_writer;
struct dh_writer *dh_writer_alloc();
void dh_writer_free(struct dh_writer *);