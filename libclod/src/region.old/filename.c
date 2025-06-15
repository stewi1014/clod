
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <clod/region.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

size_t str_append(char *restrict dst, const char *restrict src) {
	size_t i = 0;
	while (src[i] != '\0') {
		dst[i] = src[i];
		i++;
	}
	dst[i] = '\0';
	return i;
}

// returns the size of dst up to the first instance of end including end.
size_t str_copychr(char *restrict dst, const char *restrict src, char end) {
	if (dst) dst[0] = src[0];
	if (src[0] == '\0') return 0;

	size_t i = 1;
	while (src[i] != '\0' && src[i] != end) {
		if (dst) dst[i] = src[i];
		i++;
	}
	if (dst) dst[i] = '\0';
	if (src[i] == end) return i + 1;
	return i;
}

size_t str_read64(const char *restrict str, int64_t *i) {
	int old_errno = errno;
	errno = 0;
	char *end;
	int64_t val = strtoll(str, &end, 10);
	if (errno == ERANGE || end == str) {
		errno = old_errno;
		return 0;
	}
	errno = old_errno;
	if (i) *i = val;
	return (size_t)(end - str);
}

char *make_filename(char *dst, const char *prefix, const char *extension, uint8_t coord_count, const int64_t *pos, char tail) {
	dst += str_append(dst, prefix);
	dst += str_append(dst, ".");
	for (uint8_t c = 0; c < coord_count; c++)
		dst += sprintf(dst, "%"PRId64".", (pos) ? pos[c] : 0);
	dst += str_append(dst, extension);
	if (tail) {
		*dst++ = '.';
		*dst++ = tail;
		*dst++ = '\0';
	}
	return dst;
}

void parse_filename(const char *filename, char *prefix, char *extension, uint8_t *coord_count, int64_t *pos, char *tail) {
	filename += str_copychr(prefix, filename, '.');
	uint8_t i = 0;
	while (i < REGION_COORDS_MAX) {
		const size_t read_size = str_read64(filename, (pos) ? &pos[i] : nullptr);
		filename += read_size;
		if (*filename == '.') filename++;
		if (read_size == 0) break;
		i++;
	}
	if (coord_count) *coord_count = i;
	filename += str_copychr(extension, filename, '.');
	if (tail) *tail = *filename;
}
