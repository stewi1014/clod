#include "test.h"
#include <clod/hash.h>
#include <stdlib.h>

int main() {
	check(clod_crc64((uint8_t*)"abcdefg", 7) == 0x338f0e3f78cd0b9a);
	check(clod_crc32((uint8_t*)"abcdefg", 7) == 0x312a6aa6);
	check(clod_crc24((uint8_t*)"abcdefg", 7) == 0xfe0756);
	check(clod_crc16((uint8_t*)"abcdefg", 7) == 0xf90c);
	check(clod_crc8((uint8_t*)"abcdefg", 7) == 0x9f);

	return EXIT_SUCCESS;
}
