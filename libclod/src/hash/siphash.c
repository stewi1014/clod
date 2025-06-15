#include <clod/hash.h>
#include <stddef.h>
#include <stdint.h>

#define rot(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define round(v0, v1, v2, v3) do {\
	v0 += v1;\
	v1 = rot(v1, 13);\
	v1 ^= v0;\
	v0 = rot(v0, 32);\
	v2 += v3;\
	v3 = rot(v3, 16);\
	v3 ^= v2;\
	v0 += v3;\
	v3 = rot(v3, 21);\
	v3 ^= v0;\
	v2 += v1;\
	v1 = rot(v1, 17);\
	v1 ^= v2;\
	v2 = rot(v2, 32);\
} while(false)

static uint64_t read_uint64(const unsigned char *restrict data, const size_t data_size) {
	uint64_t r = 0;
	switch (data_size) {
		default: r |= (uint64_t)data[7] << 7 * 8; __attribute__((fallthrough));
		case 7:  r |= (uint64_t)data[6] << 6 * 8; __attribute__((fallthrough));
		case 6:  r |= (uint64_t)data[5] << 5 * 8; __attribute__((fallthrough));
		case 5:  r |= (uint64_t)data[4] << 4 * 8; __attribute__((fallthrough));
		case 4:  r |= (uint64_t)data[3] << 3 * 8; __attribute__((fallthrough));
		case 3:  r |= (uint64_t)data[2] << 2 * 8; __attribute__((fallthrough));
		case 2:  r |= (uint64_t)data[1] << 1 * 8; __attribute__((fallthrough));
		case 1:  r |= (uint64_t)data[0] << 0 * 8; __attribute__((fallthrough));
		case 0: return r;
	}
}

/**
 * https://www.aumasson.jp/siphash/siphash.pdf
 */
uint64_t clod_hash64(uint64_t seed, const void *data, size_t size) {
	auto in = (const unsigned char* restrict)data;
	uint64_t v0 = UINT64_C(0x736f6d6570736575) ^ seed;
	uint64_t v1 = UINT64_C(0x646f72616e646f6d) ^ seed;
	uint64_t v2 = UINT64_C(0x6c7967656e657261) ^ seed;
	uint64_t v3 = UINT64_C(0x7465646279746573) ^ seed;

	size_t i = 0;
	for (; i + 8 <= size; i += 8) {
		const uint64_t d = read_uint64(in + i, 8);
		v3 ^= d;
		round(v0, v1, v2, v3);
		round(v0, v1, v2, v3);
		v0 ^= d;
	}

	uint64_t d = read_uint64(in + i, size - i);
	d |= (uint64_t)size << (7*8);
	v3 ^= d;
	round(v0, v1, v2, v3);
	round(v0, v1, v2, v3);
	v0 ^= d;
	v2 ^= 0xee;
	round(v0, v1, v2, v3);
	round(v0, v1, v2, v3);
	round(v0, v1, v2, v3);
	round(v0, v1, v2, v3);
	return v0 ^ v1 ^ v2 ^ v3;
}
