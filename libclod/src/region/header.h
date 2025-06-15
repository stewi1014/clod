#ifndef CLOD_REGION_HEADER_H
#define CLOD_REGION_HEADER_H

#include <clod/region.h>
#include <clod/big_endian.h>
#include <stdint.h>
#include <string.h>

#include "clod/hash.h"

static_assert(CHAR_BIT == 8);

#define HEADER_MAGIC "\n\n### libclod extended region file format. ###\n\n"

#define FEATURE_KIND_MASK 0b00001111
#define FEATURE_FLAG_MASK 0b11110000

// The feature doesn't use any standard functionality like extending the header.
#define FEATURE_KIND_SIMPLE 1
// The feature extends the header, and the location and size of the extension is stored in the feature array.
#define FEATURE_KIND_EXTENDED 2
// The feature prevents implementations that don't understand it from reading.
#define FEATURE_FLAG_BLOCK_READ	 (0b01000000)
// The feature prevents implementations that don't understand it from writing.
#define FEATURE_FLAG_BLOCK_WRITE (0b10000000)

#define FEATURE_DATA_OFFSET(location) (intn_get(location) >> 12)
#define FEATURE_DATA_SIZE(location) (intn_get(location) & 0xfff)

struct feature {
	// Unique ID of the feature.
	// Zero value indicates no feature exists.
	intn24u feature;

	// Upper bits used as flags to indicate how implementations that don't understand the feature should behave.
	// Lower bits store the kind of the feature which indicates what the remaining
	// 8 bytes of feature data represent, if anything.
	beu8 mode;

	union {
		// Data for simple feature kinds.
		struct {
			// Data for the feature to use.
			intn64u data;
		} simple;

		// Data for extended header features.
		struct {
			// The high 20 bits store the location in the file of the feature's extra data,
			// and the low 12 bits store the size of the feature's extra data.
			intn32u location;
			// Data for the feature to use.
			// E.g. chunk checksum feature might store the generation here,
			// to distinguish between a write that didn't update the checksum (i.e. feature not supported),
			// or
			intn32u data;
		} extends_header;
	};
};
struct header {
	// Region file starts with an array of chunk locations.
	intn32u *chunk_locations;
	// Last modification time of each chunk.
	intn32u *chunk_timestamps;

	// libclod extended header. Allows describing features.
	// It is always placed at offset 8192 in the file.
	struct header_extended {
		// Equal to HEADER_MAGIC. Used to detect if the region file is libclod format.
		// Only the first 48 bytes are compared. The rest are reserved for now.
		char magic[128];
		// Header checksum. Includes all header data including feature data, chunk locations, etc...
		intn32u checksum;
		// Increments with every write made to the file.
		intn32u generation;
		// Sector size.
		intn32u sector_size;
		// Number of dimensions.
		beu8 dimensions;
		// Reserved space - rounds the libclod extended header size up to 4096.
		char _reserved[1024 - 128 - 4 - 4 - 4 - 1];
		// Feature list.
		// Terminated by a zeroed feature.
		struct feature features[256];
	} *extended;
};
static_assert(sizeof(struct header_extended) == 4096);

// dimension count -> region file extent mapping.
// libclod region files each store a hypercube of space,
// the width of the hypercube being given by this constant.
// Values are chosen to ensure the number of chunks in each region file is close to 1024.
static constexpr size_t extent[CLOD_REGION_DIMENSIONS_MAX + 1] = {
	[0] = 0, // invalid
	[1] = 1024,
	[2] = 32,
	[3] = 10,
	[4] = 5,
	[5] = 4,
	[6] = 3,
	[7] = 2,
	[8] = 2,
	[9] = 2,
	[10] = 2
};

// dimension count -> number of chunks in each region file.
// It is extent^dimension_count.
// put another way - it is the volume of the hypercube each region file stores.
static constexpr size_t cardinality[CLOD_REGION_DIMENSIONS_MAX + 1] = {
	[0] = 0, // invalid
	[1] = 1024,
	[2] = 1024,
	[3] = 1000,
	[4] = 625,
	[5] = 1024,
	[6] = 729,
	[7] = 128,
	[8] = 256,
	[9] = 512,
	[10] = 1024
};

static bool header_checksum(struct header *h) {
	uint32_t crc = CLOD_CRC32_INIT;
	if (h->chunk_locations != nullptr)
		crc = clod_crc32_add(crc, h->chunk_locations, 4096);
	if (h->chunk_timestamps != nullptr)
		crc = clod_crc32_add(crc, h->chunk_timestamps, 4096);
	if (h->extended != nullptr) {
		crc = clod_crc32_add(crc, h->extended, offsetof(struct header_extended, checksum));
		crc = clod_crc32_add(crc,
			(char*)h->extended + offsetof(struct header_extended, checksum) + sizeof(h->extended->checksum),
			sizeof(struct header_extended) - offsetof(struct header_extended, checksum) - sizeof(h->extended->checksum));

		for (int i = 0; i < 256 && intn_get(h->extended->features[i].feature); i++) {
			struct feature *f = &h->extended->features[i];

			if (intn_get(f->mode) & FEATURE_KIND_MASK == FEATURE_KIND_EXTENDED) {
				auto const offset = FEATURE_DATA_OFFSET(f->extends_header.location);
				auto const size = FEATURE_DATA_SIZE(f->extends_header.location);
				crc = clod_crc32_add(crc, h->extended + offset, size);
			}
		}
	}

	checksum = clod_crc32_add(checksum, , 8192);
	for (int i = 0; i < 256 && intn_get(h->extended->features[i].feature); i++) {
		if (intn_get(h->extended->features[i].mode) & FEATURE_KIND_EXTENDED) {
			auto const offset = FEATURE_DATA_OFFSET(h->extended->features[i].extends_header.location);
			auto const size = FEATURE_DATA_SIZE(h->extended->features[i].extends_header.location);
			checksum = clod_crc32_add(checksum, h->extended + offset, size);
		}
	}
}

static bool get_header(struct header *h, char *data, const size_t data_size, const uint8_t dims) {
	if (data_size < cardinality[dims] * 8) return false;

	h->chunk_locations = (void*)data;
	h->chunk_timestamps = (void*)(data + cardinality[dims] * 4);

	if (
		data_size < 8192 + sizeof(struct header_extended) ||
		strncmp(data + 8192, HEADER_MAGIC, sizeof(HEADER_MAGIC)) != 0
	) {
		h->extended = nullptr;
		return true;
	}

	h->extended = (void*)(data + 8192);

	uint32_t checksum = CLOD_CRC32_INIT;
	checksum = clod_crc32_add(checksum, data, 8192 + sizeof(struct header_extended));
	for (int i = 0; i < 256 && intn_get(h->extended->features[i].feature); i++) {
		if (intn_get(h->extended->features[i].mode) & FEATURE_KIND_EXTENDED) {
			auto const offset = FEATURE_DATA_OFFSET(h->extended->features[i].extends_header.location);
			auto const size = FEATURE_DATA_SIZE(h->extended->features[i].extends_header.location);
			if (offset + size > data_size) return false;
			checksum = clod_crc32_add(checksum, data + offset, size);
		}
		auto const offset = FEATURE_DATA_OFFSET(h.extended->features[i].);
	}

	return h;
}


#endif