#include "filename.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

void str_copy(
	char *restrict dst, const ptrdiff_t dst_max, size_t *dst_offset,
	const char *restrict src, const ptrdiff_t src_max, size_t *src_offset,
	const char delim
) {
	const size_t d = dst_offset ? *dst_offset : 0;
	const size_t s = src_offset ? *src_offset : 0;

	size_t i;
	for (i = 0; (ptrdiff_t)(i + s) < src_max && src[i + s] && src[i + s] != delim; i++)
		if ((ptrdiff_t)(i + d) < dst_max) dst[i + d] = src[i + s];

	if (dst_offset) *dst_offset = d + i;
	if (src_offset) *src_offset = s + i;
}

/**
 * Filename structure is
 * <prefix>[.<coordN>...].<extension>
 * e.g. region.-32.40.mcr, lod.8.65.2.dhd.lck
 */
size_t filename_make(
	char filename[REGION_FILENAME_MAX + 1],
	const char prefix[PREFIX_MAX],
	const char *extension,
	const int64_t *pos, const uint8_t dims
) {
	assert(dims <= CLOD_REGION_DIMENSIONS_MAX);
	memset(filename, 0, REGION_FILENAME_MAX + 1);

	size_t offset = 0;
	str_copy(filename, REGION_FILENAME_MAX, &offset, prefix, CLOD_REGION_PREFIX_MAX, nullptr, '.');
	str_copy(filename, REGION_FILENAME_MAX, &offset, ".", 1, nullptr, 0);

	for (int i = 0; i < dims; i++) {
		const size_t max_len = offset > REGION_FILENAME_MAX ? 0 : REGION_FILENAME_MAX - offset;
		char *cursor = offset > REGION_FILENAME_MAX ? filename + REGION_FILENAME_MAX : filename + offset;
		offset += (size_t)snprintf(cursor, max_len, "%"PRId64".", pos[i]);
	}

	str_copy(filename, REGION_FILENAME_MAX, &offset, extension, EXTENSION_MAX, nullptr, 0);
	assert(offset <= REGION_FILENAME_MAX);
	return offset;
}

/**
 * Parse filename.
 * Returns 0 on failure, or the size of the parsed filename.
 */
bool filename_parse_pos(const char filename[REGION_FILENAME_MAX + 1],
                      const char prefix[PREFIX_MAX],
                      const char *extension,
                      int64_t *pos, const uint8_t dims
) {
	size_t offset = 0;
	while (offset < PREFIX_MAX && prefix[offset]) {
		if (prefix[offset] != filename[offset]) return false;
		offset++;
	}

	if (filename[offset] != '.') return false;
	offset++;

	for (uint8_t i = 0; i < dims; i++) {
		char *end;
		pos[i] = strtoll(&filename[offset], &end, 10);
		if (end == filename + offset) return false;
		offset += (size_t)(end - filename) + 1;
	}

	size_t extension_offset = 0;
	while (extension_offset < EXTENSION_MAX && extension[extension_offset]) {
		if (extension[extension_offset] != filename[offset]) return false;
		extension_offset++;
		offset++;
	}

	if (filename[offset]) return false;
	return true;
}