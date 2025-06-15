#include "test.h"
#include <clod/hash.h>
#include <stdlib.h>

int main() {
	uint64_t crc64 = CLOD_CRC64_INIT;
	crc64 = clod_crc64_add(crc64, "abcd", 4);
	crc64 = clod_crc64_add(crc64, "efg", 3);
	check(clod_crc64_finalise(crc64) == clod_crc64("abcdefg", 7));

	uint32_t crc32 = CLOD_CRC32_INIT;
	crc32 = clod_crc32_add(crc32, "abcd", 4);
	crc32 = clod_crc32_add(crc32, "efg", 3);
	check(clod_crc32_finalise(crc32) == clod_crc32("abcdefg", 7));

	uint32_t crc24 = CLOD_CRC24_INIT;
	crc24 = clod_crc24_add(crc24, "abcd", 4);
	crc24 = clod_crc24_add(crc24, "efg", 3);
	check(clod_crc24_finalise(crc24) == clod_crc24("abcdefg", 7));

	uint16_t crc16 = CLOD_CRC16_INIT;
	crc16 = clod_crc16_add(crc16, "abcd", 4);
	crc16 = clod_crc16_add(crc16, "efg", 3);
	check(clod_crc16_finalise(crc16) == clod_crc16("abcdefg", 7));

	uint8_t crc8 = CLOD_CRC8_INIT;
	crc8 = clod_crc8_add(crc8, "abcd", 4);
	crc8 = clod_crc8_add(crc8, "efg", 3);
	check(clod_crc8_finalise(crc8) == clod_crc8("abcdefg", 7));

	return EXIT_SUCCESS;
}
