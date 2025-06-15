#include <clod/hash.h>
#include <stddef.h>
#include <stdint.h>

#include "crc_tables.h"

uint64_t clod_crc64_add(uint64_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *p = data;
	while (data_len--)
		crc = crc64_table[(crc >> 56) ^ *p++] ^ (crc << 8);
	return crc;
}

uint32_t clod_crc32_add(uint32_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *p = data;
	while (data_len--)
		crc = crc32_table[(crc & 0xff) ^ *p++] ^ (crc >> 8);
	return crc;
}

uint32_t clod_crc24_add(uint32_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *p = data;
	crc = crc & 0x00FFFFFF;
	while (data_len--)
		crc = (crc24_table[(crc >> 16) ^ *p++] ^ (crc << 8)) & 0x00FFFFFF;
	return crc;
}

uint16_t clod_crc16_add(uint16_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *p = data;
	while (data_len--)
		crc = crc16_table[(crc & 0xff) ^ *p++] ^ (uint16_t)(crc >> 8);
	return crc;
}

uint8_t clod_crc8_add(uint8_t crc, const void *restrict data, size_t data_len) {
	const uint8_t *p = data;
	while (data_len--)
		crc = crc8_table[(crc & 0xff) ^ *p++] ^ (uint8_t)(crc >> 8);
	return crc;
}
