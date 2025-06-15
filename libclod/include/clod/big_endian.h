/**
 * @file big_endian.h
 * @breif A silly header-only library for interacting with unaligned big-endian integers.
 */

#ifndef CLOD_BIG_ENDIAN_H
#define CLOD_BIG_ENDIAN_H

#include <limits.h>
#include <stdint.h>

static_assert(CHAR_BIT == 8);
static_assert(__STDC_VERSION__ >= 202311L); // shifting signed integers left.
static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);

typedef struct { uint8_t _b[1]; } bei8 ;
typedef struct { uint8_t _b[2]; } bei16;
typedef struct { uint8_t _b[3]; } bei24;
typedef struct { uint8_t _b[4]; } bei32;
typedef struct { uint8_t _b[5]; } bei40;
typedef struct { uint8_t _b[6]; } bei48;
typedef struct { uint8_t _b[7]; } bei56;
typedef struct { uint8_t _b[8]; } bei64;

#define bei8(val)  bei8_enc (val)
#define bei16(val) bei16_enc(val)
#define bei24(val) bei24_enc(val)
#define bei32(val) bei32_enc(val)
#define bei40(val) bei40_enc(val)
#define bei48(val) bei48_enc(val)
#define bei56(val) bei56_enc(val)
#define bei64(val) bei64_enc(val)

static bei8  bei8_enc (const int8_t  val) { bei8  n; n._b[0] = val; return n; }
static bei16 bei16_enc(const int16_t val) { bei16 n; n._b[0] = val >> 8 ; n._b[1] = val; return n; }
static bei24 bei24_enc(const int32_t val) { bei24 n; n._b[0] = val >> 16; n._b[1] = val >> 8 ; n._b[2] = val; return n; }
static bei32 bei32_enc(const int32_t val) { bei32 n; n._b[0] = val >> 24; n._b[1] = val >> 16; n._b[2] = val >> 8 ; n._b[3] = val; return n; }
static bei40 bei40_enc(const int64_t val) { bei40 n; n._b[0] = val >> 32; n._b[1] = val >> 24; n._b[2] = val >> 16; n._b[3] = val >> 8 ; n._b[4] = val; return n; }
static bei48 bei48_enc(const int64_t val) { bei48 n; n._b[0] = val >> 40; n._b[1] = val >> 32; n._b[2] = val >> 24; n._b[3] = val >> 16; n._b[4] = val >> 8 ; n._b[5] = val; return n; }
static bei56 bei56_enc(const int64_t val) { bei56 n; n._b[0] = val >> 48; n._b[1] = val >> 40; n._b[2] = val >> 32; n._b[3] = val >> 24; n._b[4] = val >> 16; n._b[5] = val >> 8 ; n._b[6] = val; return n; }
static bei64 bei64_enc(const int64_t val) { bei64 n; n._b[0] = val >> 56; n._b[1] = val >> 48; n._b[2] = val >> 40; n._b[3] = val >> 32; n._b[4] = val >> 24; n._b[5] = val >> 16; n._b[6] = val >> 8 ; n._b[7] = val; return n; }

static int8_t  bei8_dec (const bei8  n) { return (int8_t)  n._b[0]; }
static int16_t bei16_dec(const bei16 n) { return (int16_t)((int16_t)(int8_t)n._b[0] << 8  | (uint16_t)n._b[1]); }
static int32_t bei24_dec(const bei24 n) { return (int32_t)((int32_t)(int8_t)n._b[0] << 16 | (uint32_t)n._b[1] << 8  | (uint32_t)n._b[2]); }
static int32_t bei32_dec(const bei32 n) { return (int32_t)((int32_t)(int8_t)n._b[0] << 24 | (uint32_t)n._b[1] << 16 | (uint32_t)n._b[2] << 8  | (uint32_t)n._b[3]); }
static int64_t bei40_dec(const bei40 n) { return (int64_t)((int64_t)(int8_t)n._b[0] << 32 | (uint64_t)n._b[1] << 24 | (uint64_t)n._b[2] << 16 | (uint64_t)n._b[3] << 8  | (uint64_t)n._b[4]); }
static int64_t bei48_dec(const bei48 n) { return (int64_t)((int64_t)(int8_t)n._b[0] << 40 | (uint64_t)n._b[1] << 32 | (uint64_t)n._b[2] << 24 | (uint64_t)n._b[3] << 16 | (uint64_t)n._b[4] << 8  | (uint64_t)n._b[5]); }
static int64_t bei56_dec(const bei56 n) { return (int64_t)((int64_t)(int8_t)n._b[0] << 48 | (uint64_t)n._b[1] << 40 | (uint64_t)n._b[2] << 32 | (uint64_t)n._b[3] << 24 | (uint64_t)n._b[4] << 16 | (uint64_t)n._b[5] << 8  | (uint64_t)n._b[6]); }
static int64_t bei64_dec(const bei64 n) { return (int64_t)((int64_t)(int8_t)n._b[0] << 56 | (uint64_t)n._b[1] << 48 | (uint64_t)n._b[2] << 40 | (uint64_t)n._b[3] << 32 | (uint64_t)n._b[4] << 24 | (uint64_t)n._b[5] << 16 | (uint64_t)n._b[6] << 8  | (uint64_t)n._b[7]); }

typedef struct { uint8_t _b[1]; } beu8 ;
typedef struct { uint8_t _b[2]; } beu16;
typedef struct { uint8_t _b[3]; } beu24;
typedef struct { uint8_t _b[4]; } beu32;
typedef struct { uint8_t _b[5]; } beu40;
typedef struct { uint8_t _b[6]; } beu48;
typedef struct { uint8_t _b[7]; } beu56;
typedef struct { uint8_t _b[8]; } beu64;

#define beu8(val)  beu8_enc (val)
#define beu16(val) beu16_enc(val)
#define beu24(val) beu24_enc(val)
#define beu32(val) beu32_enc(val)
#define beu40(val) beu40_enc(val)
#define beu48(val) beu48_enc(val)
#define beu56(val) beu56_enc(val)
#define beu64(val) beu64_enc(val)

static beu8  beu8_enc (const uint8_t  val) { beu8  n; n._b[0] = val; return n; }
static beu16 beu16_enc(const uint16_t val) { beu16 n; n._b[0] = val >> 8 ; n._b[1] = val; return n; }
static beu24 beu24_enc(const uint32_t val) { beu24 n; n._b[0] = val >> 16; n._b[1] = val >> 8 ; n._b[2] = val; return n; }
static beu32 beu32_enc(const uint32_t val) { beu32 n; n._b[0] = val >> 24; n._b[1] = val >> 16; n._b[2] = val >> 8 ; n._b[3] = val; return n; }
static beu40 beu40_enc(const uint64_t val) { beu40 n; n._b[0] = val >> 32; n._b[1] = val >> 24; n._b[2] = val >> 16; n._b[3] = val >> 8 ; n._b[4] = val; return n; }
static beu48 beu48_enc(const uint64_t val) { beu48 n; n._b[0] = val >> 40; n._b[1] = val >> 32; n._b[2] = val >> 24; n._b[3] = val >> 16; n._b[4] = val >> 8 ; n._b[5] = val; return n; }
static beu56 beu56_enc(const uint64_t val) { beu56 n; n._b[0] = val >> 48; n._b[1] = val >> 40; n._b[2] = val >> 32; n._b[3] = val >> 24; n._b[4] = val >> 16; n._b[5] = val >> 8 ; n._b[6] = val; return n; }
static beu64 beu64_enc(const uint64_t val) { beu64 n; n._b[0] = val >> 56; n._b[1] = val >> 48; n._b[2] = val >> 40; n._b[3] = val >> 32; n._b[4] = val >> 24; n._b[5] = val >> 16; n._b[6] = val >> 8 ; n._b[7] = val; return n; }

static uint8_t  beu8_dec (const beu8  n) { return (uint8_t)n._b[0]; }
static uint16_t beu16_dec(const beu16 n) { return (uint16_t)((uint16_t)n._b[0] << 8  | (uint16_t)n._b[1]); }
static uint32_t beu24_dec(const beu24 n) { return (uint32_t)((uint32_t)n._b[0] << 16 | (uint32_t)n._b[1] << 8  | (uint32_t)n._b[2]); }
static uint32_t beu32_dec(const beu32 n) { return (uint32_t)((uint32_t)n._b[0] << 24 | (uint32_t)n._b[1] << 16 | (uint32_t)n._b[2] << 8  | (uint32_t)n._b[3]); }
static uint64_t beu40_dec(const beu40 n) { return (uint64_t)((uint64_t)n._b[0] << 32 | (uint64_t)n._b[1] << 24 | (uint64_t)n._b[2] << 16 | (uint64_t)n._b[3] << 8  | (uint64_t)n._b[4]); }
static uint64_t beu48_dec(const beu48 n) { return (uint64_t)((uint64_t)n._b[0] << 40 | (uint64_t)n._b[1] << 32 | (uint64_t)n._b[2] << 24 | (uint64_t)n._b[3] << 16 | (uint64_t)n._b[4] << 8  | (uint64_t)n._b[5]); }
static uint64_t beu56_dec(const beu56 n) { return (uint64_t)((uint64_t)n._b[0] << 48 | (uint64_t)n._b[1] << 40 | (uint64_t)n._b[2] << 32 | (uint64_t)n._b[3] << 24 | (uint64_t)n._b[4] << 16 | (uint64_t)n._b[5] << 8  | (uint64_t)n._b[6]); }
static uint64_t beu64_dec(const beu64 n) { return (uint64_t)((uint64_t)n._b[0] << 56 | (uint64_t)n._b[1] << 48 | (uint64_t)n._b[2] << 40 | (uint64_t)n._b[3] << 32 | (uint64_t)n._b[4] << 24 | (uint64_t)n._b[5] << 16 | (uint64_t)n._b[6] << 8  | (uint64_t)n._b[7]); }

typedef struct { uint8_t _b[4]; } bef32;
typedef struct { uint8_t _b[8]; } bef64;

#define bef32(val) bef32_enc(val)
#define bef64(val) bef64_enc(val)

static bef32 bef32_enc(const float  f) { const union { float  f; uint32_t i; } u = { f }; bef32 n; n._b[0] = u.i >> 24; n._b[1] = u.i >> 16; n._b[2] = u.i >> 8 ; n._b[3] = u.i; return n; }
static bef64 bef64_enc(const double f) { const union { double f; uint64_t i; } u = { f }; bef64 n; n._b[0] = u.i >> 56; n._b[1] = u.i >> 48; n._b[2] = u.i >> 40; n._b[3] = u.i >> 32; n._b[4] = u.i >> 24; n._b[5] = u.i >> 16; n._b[6] = u.i >> 8; n._b[7] = u.i; return n; }

static float  bef32_dec(const bef32 n) { const union { float  f; uint32_t i; } u = { .i = (uint32_t)n._b[0] << 24 | (uint32_t)n._b[1] << 16 | (uint32_t)n._b[2] << 8  | (uint32_t)n._b[3] }; return u.f; }
static double bef64_dec(const bef64 n) { const union { double f; uint64_t i; } u = { .i = (uint64_t)n._b[0] << 56 | (uint64_t)n._b[1] << 48 | (uint64_t)n._b[2] << 40 | (uint64_t)n._b[3] << 32 | (uint64_t)n._b[4] << 24 | (uint64_t)n._b[5] << 16 | (uint64_t)n._b[6] << 8 | (uint64_t)n._b[7] }; return u.f; }

#define be(be) _Generic((be),\
	bei8:  bei8_dec ,\
	bei16: bei16_dec,\
	bei24: bei24_dec,\
	bei32: bei32_dec,\
	bei40: bei40_dec,\
	bei48: bei48_dec,\
	bei56: bei56_dec,\
	bei64: bei64_dec,\
	beu8:  beu8_dec ,\
	beu16: beu16_dec,\
	beu24: beu24_dec,\
	beu32: beu32_dec,\
	beu40: beu40_dec,\
	beu48: beu48_dec,\
	beu56: beu56_dec,\
	beu64: beu64_dec,\
	bef32: bef32_dec,\
	bef64: bef64_dec \
)(be)

#endif
