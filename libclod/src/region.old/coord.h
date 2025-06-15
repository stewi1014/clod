#ifndef COORD_H
#define COORD_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/** division the way god intended */
static struct divi64 { int64_t quot; int64_t rem; } divi64(const int64_t x, const int64_t divisor) {
	struct divi64 res;
	res.quot = x / divisor;
	res.rem = x % divisor;

	if (res.rem != 0 && res.rem < 0 != divisor < 0) {
		res.rem += divisor;
		res.quot--;
	}

	return res;
}

#define vec_equal(vec1, vec2) (memcmp((vec1), (vec2), sizeof(int64_t) * O_DIMS) == 0)
#define vec_cpy(dst, src) (memcpy((dst), (src), sizeof(int64_t) * O_DIMS))
#define vec_div(dst, vec, divisor) (vec_div_ex(dst, nullptr, vec, divisor, O_DIMS))
#define vec_mod(dst, vec, divisor) (vec_div_ex(nullptr, dst, vec, divisor, O_DIMS))

static void vec_div_ex(int64_t *quot, int64_t *rem, const int64_t *vec, const int64_t divisor, const uint8_t dims) {
	for (uint32_t i = 0; i < dims; i++) {
		const struct divi64 res = divi64(vec[i], divisor);
		if (quot) quot[i] = res.quot;
		if (rem) rem[i] = res.rem;
	}
}

static uint64_t vec_lsb_pack(const uint8_t bits, const int64_t *vec, const uint8_t n) {
	uint64_t res = 0;
	for (uint8_t i = 0; i * n < bits; i++)
		for (uint8_t j = 0; j < n && i * n + j < bits; j++)
			res |= ((uint64_t)vec[j] >> i & 1) << (i * n + j);
	return res;
}

#endif
