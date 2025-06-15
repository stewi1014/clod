#ifndef CLOD_REGION_FILENAME_H
#define CLOD_REGION_FILENAME_H

#include <string.h>
#include <clod/region.h>

#define PREFIX_MAX CLOD_REGION_PREFIX_MAX
#define EXTENSION_MAX (CLOD_REGION_EXTENSION_MAX + 4)
#define REGION_FILENAME_MAX (PREFIX_MAX + 1 + CLOD_REGION_DIMENSIONS_MAX * 21 + EXTENSION_MAX)
static_assert(REGION_FILENAME_MAX == 255);

/**
 * Writes to filename, returns string length.
 * Clears all remaining bytes in the filename.
 */
size_t filename_make(
	char filename[REGION_FILENAME_MAX + 1],
	const char prefix[PREFIX_MAX],
	const char *extension,
	const int64_t *pos, uint8_t dims
);

/**
 * Writes to pos, returns true on success.
 * If any details don't match it returns false.
 */
bool filename_parse_pos(
	const char filename[REGION_FILENAME_MAX + 1],
	const char prefix[PREFIX_MAX],
	const char *extension,
	int64_t *pos, uint8_t dims
);

#endif
