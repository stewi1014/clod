/**
 * @defgroup nbt nbt.h
 *
 * @file nbt.h
 * This header defines methods for dealing with nbt data.
 *
 * @paragraph Motivations and Goals
 *
 * The primary limitation of nbt data parsing is figuring out where the tags actually are.
 * To find a given tag, every tag before it must have its size calculated and summed.
 * This process is slow when you have 60KB+ of nbt data to go through and your method
 * is doing silly nonsense like using the stack.
 * As such, @link nbt_step @endlink is the primary method of the library.
 * It calculates the size of a tag, and can do it *fast*.
 * As in fast enough that you can do it a few thousand times per interaction in a responsive application.
 * This means that a lot of extra code, caching, memory usage and complexity can be avoided for many use cases.
 *
 * For serialising data, the methods are also close to optimal, but with a caveat.
 * Not all use cases actually need to serialise the nbt data - and doing nothing is faster than something.
 * For example, if your code changes the name of an NBT tag a ~hundred thousand times it may be worth
 * caching those changes in an intermediate data structure, and only serialising the result at the end.
 * This library does not aim to provide any intermediate data structure.
 *
 * @paragraph Writing (unfinished)
 *
 * The end argument passed to read methods are used to ensure the buffer is not overrun.
 * Not so with write methods.
 *
 * Write methods take an end that indicates the size of existing valid nbt data - *not* the size of the buffer.
 * Write methods will write *as much as needed* and do not have a buffer overrun failure condition.
 * Users of write methods are responsible for ensuring the buffer is large enough to hold all written data,
 * and while how much data a write method will write is usually self-apparent and straightforward to preempt,
 * it can be explicitly validated by calling the method with a null tag/payload.
 * The method will then return the number of bytes it would have written into payload.
 * When given a non-null buffer to write into, write methods return how much
 * they grew or shrank the nbt data in the buffer.
 */

// ReSharper disable CppObjectMemberMightNotBeInitialized
// ReSharper disable CppIdenticalOperandsInBinaryExpression
#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * Types of tag, including the pseudo-tag NBT_NULL, and error indicator NBT_INVALID.
 */
typedef enum nbt_type {
    NBT_NULL        = 0,

    NBT_BYTE        = 1,
    NBT_SHORT       = 2,
    NBT_INT         = 3,
    NBT_LONG        = 4,
    NBT_FLOAT       = 5,
    NBT_DOUBLE      = 6,
    NBT_BYTE_ARRAY  = 7,
    NBT_STRING      = 8,
    NBT_LIST        = 9,
    NBT_COMPOUND    = 10,
    NBT_INT_ARRAY   = 11,
    NBT_LONG_ARRAY  = 12,

    NBT_INVALID     = 1 << 31,
    NBT_ANY_INTEGER = NBT_INVALID + 1,
    NBT_ANY_NUMBER  = NBT_INVALID + 2,
} nbt_type;

#define nbt_type_is_valid(type) (NBT_BYTE <= (type) && (type) <= NBT_LONG_ARRAY)
#define nbt_type_is_dynamic(type) (NBT_BYTE_ARRAY <= (type) && (type) <= NBT_LONG_ARRAY)
#define nbt_type_is_integer(type) (NBT_BYTE <= (type) && (type) <= NBT_LONG)
#define nbt_type_is_number(type) (NBT_BYTE <= (type) && (type) <= NBT_DOUBLE)

/**
 * Get string value of type.
 * @param type Type to return string value for.
 * @return String representation of the nbt type. i.e. NBT_INT.
 */
char *nbt_type_string(nbt_type type);

/**
 * Get the size of a payload really quickly.
 * It does not read past end.
 *
 * Not clearly indicating to the caller if data is malformed is an intentional design choice.
 * Complex data analysis to discern the likelihood that a given set of nbt data has been
 * modified from its original state is an insane alternative to just using a checksum,
 * out of scope for this library, and is at best an unreliable solution.
 *
 * @param payload Buffer containing payload data.
 * @param end End of the buffer.
 * @param type Type of payload.
 * @return The size of the payload.
 *  May return 0 or > (end - payload) on malformed data.
 */
size_t nbt_payload_step(const char *restrict payload, const char *end, nbt_type type);

/**
 * Get the size of a tag really quickly.
 * It does not read past end.
 *
 * Not clearly indicating to the caller if data is malformed is an intentional design choice.
 * Complex data analysis to discern the likelihood that a given set of nbt data has been
 * modified from its original state is an insane alternative to just using a checksum,
 * out of scope for this library, and is at best an unreliable solution.
 *
 * @param tag Buffer containing tag data.
 * @param end End of the buffer.
 * @return The size of the tag.
 *  May return 0 or > (end - payload) on malformed data.
 */
static inline size_t nbt_step(const char *restrict tag, const char *end) {
    if (end - tag < 3 || !nbt_type_is_valid(tag[0])) return 0;
    const uint16_t name_size = (uint8_t)tag[1] << 8 | (uint8_t)tag[2];
    return nbt_payload_step(tag + 3 + name_size, end, tag[0]);
}

/**
 *
 * @param payload Buffer containing compound payload.
 * @param end End of the buffer.
 * @param name Name of tag.
 * @param name_size Size of name.
 * @param type Type of the tag.
 * @param dest Pointer to value to read into.
 * @param ... Null terminated name, name_size, type and dest repeating for each tag to read.
 * @return The size of the payload.
 */
size_t nbt_named(const char *restrict payload, const char *end,
                 const char *name, size_t name_size, nbt_type type, void *dest,
                 ...
);

/**
 * Read a tag.
 * @param tag Buffer containing tag.
 * @param end End of buffer.
 * @param name Tag name.
 * @param name_size Tag name size.
 * @return Type of tag.
 */
static inline nbt_type nbt_read_tag(const char *restrict tag, const char *end, const char **name, size_t *name_size) {
    if (end - tag < 3 || !nbt_type_is_valid(tag[0])) return NBT_INVALID;

    const uint16_t ns = (uint8_t)tag[1] << 8 | (uint8_t)tag[2];
    if (end - tag < 3 + ns) return NBT_INVALID;

    if (name) *name = tag + 3;
    if (name_size) *name_size = ns;

    return tag[0];
}

/**
 * Write a tag.
 * @param tag Buffer to write tag into.
 * @param end End of existing NBT data.
 * @param type Type of tag.
 * @param name Name of tag.
 * @param name_size Size of tag name.
 * @return Change in size of NBT data.
 */
ptrdiff_t nbt_write_tag(char *restrict tag, const char *end, nbt_type type, const char *name, size_t name_size);

/**
 * Read a byte payload.
 * @param payload Buffer containing payload.
 * @param end End of buffer.
 * @return Parsed value.
 */
static inline int8_t nbt_read_byte(const char *restrict payload, const char *end) {
    if (!payload || !end || end - payload < 1) return 0;
    return payload[0];
}

/**
 * Write a byte value.
 * @param payload Buffer to write into.
 * @param end End of existing NBT data.
 * @param value Value to serialise.
 * @return Change in size of NBT data.
 */
static inline ptrdiff_t nbt_write_byte(char *restrict payload, const char *end, const int8_t value) {
    if (!payload) return 1;
    if (end && end < payload) return 0;
    payload[0] = value;
    if (!end || end == payload) return 1;
    return 0;
}

/**
 * Read a short payload.
 * @param payload Buffer containing payload.
 * @param end End of buffer.
 * @return Parsed value.
 */
static inline int16_t nbt_read_short(const char *restrict payload, const char *end) {
    if (!payload || !end || end - payload < 2) return 0;
    return
        (uint16_t)payload[0] << (1 * 8) |
        (uint16_t)payload[1] << (0 * 8) ;
}

static inline ptrdiff_t nbt_write_short(char *restrict payload, const char *end, const int16_t value) {
    if (!payload) return 2;
    if (end && end < payload) return 0;
    payload[0] = value >> (1 * 8);
    payload[1] = value >> (0 * 8);
    if (!end || end == payload) return 2;
    return 0;
}

/**
 * Read an int payload.
 * @param payload Buffer containing payload.
 * @param end End of buffer.
 * @return Parsed value.
 */
static inline int32_t nbt_read_int(const char *restrict payload, const char *end) {
    if (!payload || !end || end - payload < 4) return 0;
    return
        (uint32_t)payload[0] << (3 * 8) |
        (uint32_t)payload[1] << (2 * 8) |
        (uint32_t)payload[2] << (1 * 8) |
        (uint32_t)payload[3] << (0 * 8) ;
}

static inline ptrdiff_t nbt_write_int(char *restrict payload, const char *end, const int32_t value) {
    if (!payload) return 4;
    if (end && end < payload) return 0;
    payload[0] = value >> (3 * 8);
    payload[1] = value >> (2 * 8);
    payload[2] = value >> (1 * 8);
    payload[3] = value >> (0 * 8);
    if (!end || end == payload) return 4;
    return 0;
}

/**
 * Read a long payload.
 * @param payload Buffer containing payload.
 * @param end End of buffer.
 * @return Parsed value.
 */
static inline int64_t nbt_read_long(const char *restrict payload, const char *end) {
    if (!payload || !end || end - payload < 8) return 0;
    return
        (uint64_t)payload[0] << (7 * 8) |
        (uint64_t)payload[1] << (6 * 8) |
        (uint64_t)payload[2] << (5 * 8) |
        (uint64_t)payload[3] << (4 * 8) |
        (uint64_t)payload[4] << (3 * 8) |
        (uint64_t)payload[5] << (2 * 8) |
        (uint64_t)payload[6] << (1 * 8) |
        (uint64_t)payload[7] << (0 * 8) ;
}

static inline ptrdiff_t nbt_write_long(char *restrict payload, const char *end, const int64_t value) {
    if (!payload) return 8;
    if (end && end < payload) return 0;
    payload[0] = value >> (7 * 8);
    payload[1] = value >> (6 * 8);
    payload[2] = value >> (5 * 8);
    payload[3] = value >> (4 * 8);
    payload[4] = value >> (3 * 8);
    payload[5] = value >> (2 * 8);
    payload[6] = value >> (1 * 8);
    payload[7] = value >> (0 * 8);
    if (!end || end == payload) return 8;
    return 0;
}

/**
 * Read a float payload.
 * @param payload Buffer containing payload.
 * @param end End of buffer.
 * @return Parsed value.
 */
static inline float nbt_read_float(const char *restrict payload, const char *end) {
    if (!payload || !end || end - payload < 4) return 0.0f / 0.0f;
    union { float f; char byte[4]; } u;
    u.byte[3] = payload[0];
    u.byte[2] = payload[1];
    u.byte[1] = payload[2];
    u.byte[0] = payload[3];
    return u.f;
}

static inline ptrdiff_t nbt_write_float(char *restrict payload, const char *end, const float value) {
    if (!payload) return 4;
    if (end && end < payload) return 0;
    union { float f; char byte[4]; } u;
    u.f = value;
    payload[3] = u.byte[0];
    payload[2] = u.byte[1];
    payload[1] = u.byte[2];
    payload[0] = u.byte[3];
    if (!end || end == payload) return 4;
    return 0;
}

/**
 * Read a double payload.
 * @param payload Buffer containing payload.
 * @param end End of buffer.
 * @return Parsed value.
 */
static inline double nbt_read_double(const char *restrict payload, const char *end) {
    if (!payload || !end || end - payload < 8) return 0.0 / 0.0;
    union { double d; char byte[8]; } u;
    u.byte[7] = payload[0];
    u.byte[6] = payload[1];
    u.byte[5] = payload[2];
    u.byte[4] = payload[3];
    u.byte[3] = payload[4];
    u.byte[2] = payload[5];
    u.byte[1] = payload[6];
    u.byte[0] = payload[7];
    return u.d;
}

static inline ptrdiff_t nbt_write_double(char *restrict payload, const char *end, const double value) {
    if (!payload) return 8;
    if (end && end < payload) return 0;
    union { double d; char byte[8]; } u;
    u.d = value;
    payload[7] = u.byte[0];
    payload[6] = u.byte[1];
    payload[5] = u.byte[2];
    payload[4] = u.byte[3];
    payload[3] = u.byte[4];
    payload[2] = u.byte[5];
    payload[1] = u.byte[6];
    payload[0] = u.byte[7];
    if (!end || end == payload) return 8;
    return 0;
}

/**
 * Read a byte array payload.
 * @param[in] payload Buffer containing payload.
 * @param[in] end End of buffer.
 * @param[out] size Size of the byte array.
 * @return Parsed value.
 */
static inline const char *nbt_read_bytea(const char *restrict payload, const char *end, size_t *size) {
    if (!payload || !end || end - payload < 4) return nullptr;
    const int32_t s =
        (uint8_t)payload[0] << (3 * 8) |
        (uint8_t)payload[1] << (2 * 8) |
        (uint8_t)payload[2] << (1 * 8) |
        (uint8_t)payload[3] << (0 * 8) ;

    if (s < 0 || end - payload < 4 + s) return nullptr;
    *size = (size_t)s;
    return payload + 4;
}

ptrdiff_t nbt_write_bytea(char *restrict payload, const char *end, const char *bytes, size_t bytes_size);

/**
 * Read a string payload.
 * @param[in] payload Buffer containing payload.
 * @param[in] end End of buffer.
 * @param[out] size Size of the string.
 * @return Parsed value.
 */
static inline const char *nbt_read_string(const char *restrict payload, const char *end, size_t *size) {
    if (!payload || !end || end - payload < 2) return nullptr;
    const int32_t s =
        (uint8_t)payload[0] << (1 * 8) |
        (uint8_t)payload[1] << (0 * 8) ;

    if (s < 0 || end - payload < 2 + s) return nullptr;
    *size = (size_t)s;
    return payload + 2;
}

ptrdiff_t nbt_write_string(char *restrict payload, const char *end, const char *str, size_t str_size);

/**
 * Read a string payload.
 * @param[in] payload Buffer containing payload.
 * @param[in] end End of buffer.
 * @param[out] type Type of list elements.
 * @param[out] size Size of the string.
 * @return Parsed value.
 */
static inline const char *nbt_read_list(const char *restrict payload, const char *end, nbt_type *type, size_t *size) {
    if (!payload || !end || end - payload < 5) return nullptr;

    const nbt_type t =
        nbt_type_is_valid(payload[0]) || payload[0] == NBT_NULL ?
        payload[0] : NBT_INVALID;

    const int32_t s =
        (uint8_t)payload[1] << (3 * 8) |
        (uint8_t)payload[2] << (2 * 8) |
        (uint8_t)payload[3] << (1 * 8) |
        (uint8_t)payload[4] << (0 * 8) ;

    if (type) *type = t;
    if (size) *size = s;
    if (s < 0 || t == NBT_INVALID) return nullptr;
    return payload + 5;
}

ptrdiff_t nbt_write_list(char *restrict payload, const char *end, const char *list, nbt_type etype, size_t list_size);


static inline const char *nbt_read_inta(const char *restrict payload, const char *end, size_t *size) {
    if (!payload || !end || end - payload < 4) return nullptr;

    const int32_t s =
        (uint8_t)payload[0] << (3 * 8) |
        (uint8_t)payload[1] << (2 * 8) |
        (uint8_t)payload[2] << (1 * 8) |
        (uint8_t)payload[3] << (0 * 8) ;

    if (size) *size = s;
    if (s < 0 || end - payload < 4 + s * 4) return nullptr;
    return payload + 4;
}

ptrdiff_t nbt_write_inta(char *restrict payload, const char *end, const char *array, size_t array_size);

static inline const char *nbt_read_longa(const char *restrict payload, const char *end, size_t *size) {
    if (!payload || !end || end - payload < 8) return nullptr;

    const int32_t s =
        (uint8_t)payload[0] << (3 * 8) |
        (uint8_t)payload[1] << (2 * 8) |
        (uint8_t)payload[2] << (1 * 8) |
        (uint8_t)payload[3] << (0 * 8) ;

    if (size) *size = s;
    if (s < 0 || end - payload < 4 + s * 8) return nullptr;
    return payload + 4;
}

ptrdiff_t nbt_write_longa(char *restrict payload, const char *end, const char *array, size_t array_size);
