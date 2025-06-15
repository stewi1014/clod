/**
 * @file clod/hash.h
 */
#ifndef CLOD_HASH_H
#define CLOD_HASH_H

#include <clod/lib.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Generate a 64-bit hash aimed at maximising speed and uniformity of entropy across the 64-bit range.
 *
 * Currently, it's an implementation of SipHash, and I thank those who have researched and developed SipHash,
 * and the original designers Jean-Philippe Aumasson and Daniel J. Bernstein.
 *
 * @param[in] seed 64 bits of seed.
 * @param[in] data Value to be hashed.
 * @param[in] size Size of data.
 * @return 64-bit hash value.
 */
CLOD_API CLOD_USE_RETURN CLOD_CONST CLOD_NONNULL(2)
uint64_t
clod_hash64(uint64_t seed, const void *data, size_t size);

/**
 * @defgroup crc CRC checksums.
 * @brief Methods for calculating CRC checksums.
 *
 * The general idea is each CRC variant provides an init/add/finalise method.
 * Initialisation is a constant the checksum is initialised to,
 * adding is the meat of the implementation and updates the checksum with new data, and
 * finalising is xoring the checksum against a second constant to produce a final output.
 *
 * The methods use lookup tables. See libclod/src/hash/crc_tables_generate.c for how they are generated,
 * and libclod/src/hash/crc.c for how they are used.
 *
 * @code
 * crc = CLOD_<ALG>_INIT;
 * crc = clod_<alg>_add(crc, data, size);
 * crc = clod_<alg>_add(crc, data2, size2);
 * crc = clod_<alg>_add(crc, data3, size3);
 * crc = CLOD_<ALG>_FINALISE(crc);
 * @endcode
 *
 * Often, using streaming methods is an annoying complexity,
 * so a method for computing the checksum of a single blob of data is provided.
 *
 * @code
 * crc = clod_<alg>(data, size)
 * @endcode
 *
 * In the modern abstracted world we tend to mostly deal with big-or-nothing corruption.
 * For this case, CRC's main strength is lost - very reliably detecting small changes in the order of bits and bytes to data.
 * Alas, there is no way to perfectly detect arbitrary-sized modification without comparing with the original.
 * Still, it remains a decent hash function for ensuring data integrity at any scale,
 * and with enough hash bits, anything can become impossible.
 *
 * @{
 */

/**
 * Polynomial: 0x42F0E1EBA9EA3693
 * Reflected: false
 */
CLOD_API CLOD_USE_RETURN CLOD_CONST CLOD_NONNULL(2)
uint64_t
clod_crc64_add(uint64_t crc, const void *data, size_t data_len);
#define CLOD_CRC64_INIT UINT64_C(0)
#define clod_crc64_finalise(crc) ((uint64_t)((uint64_t)crc ^ UINT64_C(0)))
#define clod_crc64(data, size) clod_crc64_finalise(clod_crc64_add(CLOD_CRC64_INIT, data, size))

/**
 * Polynomial: 0x4C11DB7
 * Reflected: true
 */
CLOD_API CLOD_USE_RETURN CLOD_CONST CLOD_NONNULL(2)
uint32_t
clod_crc32_add(uint32_t crc, const void *data, size_t data_len);
#define CLOD_CRC32_INIT UINT32_C(0xFFFFFFFF)
#define clod_crc32_finalise(crc) ((uint32_t)((uint32_t)crc ^ UINT32_C(0xFFFFFFFF)))
#define clod_crc32(data, size) clod_crc32_finalise(clod_crc32_add(CLOD_CRC32_INIT, data, size))

/**
 * Polynomial: 0x864CFB
 * Reflected: false
 */
CLOD_API CLOD_USE_RETURN CLOD_CONST CLOD_NONNULL(2)
uint32_t
clod_crc24_add(uint32_t crc, const void *data, size_t data_len);
#define CLOD_CRC24_INIT UINT32_C(0xB704CE)
#define clod_crc24_finalise(crc) ((uint32_t)((uint32_t)crc ^ UINT32_C(0x0)))
#define clod_crc24(data, size) clod_crc24_finalise(clod_crc24_add(CLOD_CRC24_INIT, data, size))

/**
 * Polynomial: 0x1021
 * Reflected: true
 */
CLOD_API CLOD_USE_RETURN CLOD_CONST CLOD_NONNULL(2)
uint16_t
clod_crc16_add(uint16_t crc, const void *data, size_t data_len);
#define CLOD_CRC16_INIT UINT16_C(0)
#define clod_crc16_finalise(crc) ((uint16_t)((uint16_t)crc ^ UINT16_C(0)))
#define clod_crc16(data, size) clod_crc16_finalise(clod_crc16_add(CLOD_CRC16_INIT, data, size))

/**
 * Polynomial: 0x7
 * Reflected: false
 */
CLOD_API CLOD_USE_RETURN CLOD_CONST CLOD_NONNULL(2)
uint8_t
clod_crc8_add(uint8_t crc, const void *data, size_t data_len);
#define CLOD_CRC8_INIT UINT8_C(0)
#define clod_crc8_finalise(crc) ((uint8_t)((uint8_t)crc ^ UINT8_C(0)))
#define clod_crc8(data, size) clod_crc8_finalise(clod_crc8_add(CLOD_CRC8_INIT, data, size))

/** @} */

#endif
