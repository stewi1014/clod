/**
 *
 * @defgroup nbt nbt.h
 * @brief NBT visiting and parsing
 *
 * @{
 * @file nbt.h
 * 
 * Aims to make it straightforward to operate on serialised NBT data, but does not offer any intermediate representation of NBT data.
 * 
 * Methods operate directly on the serialised data, which makes it fast as all get-out for parsing and making changes to serialised data.
 * However, some use cases do not need changes to be serialised,
 * and can benefit from storing changes in an intermediate data format - this library is not for those use cases.
 * 
 * This approach also comes with an ergonomic downside;
 * Nuances of the NBT format are not abstracted away by this library, and using it can be verbose.
 * So, if you're going to use it, make sure you're aware of how the NBT format works.
 */

#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __GNUC__

/// @private
#define __nbt_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))

/// @private
#define __nbt_nonnull_size(str, size) __attribute__((nonnull_if_nonzero(str, size)))

#else

/// @private
#define __nbt_nonnull(...)

/// @private
#define __nbt_nonnull_size(str, size)

#endif

// disable asserts in release builds.
#ifndef NDEBUG

/// @private
#define __nbt_assert(expr) assert(expr)

#else

/// @private
#define __nbt_assert(expr)

#endif

/// @private
#define __nbt_has_data(ptr, end, size) ((size) >= 0 && ((char*)(end) == nullptr || (ptr) <= ((char*)(end)-(size))))



//=======================//
// Primary Functionality //
//=======================//

/**
 * nbt_step returns the byte following the tag,
 * which for valid NBT data is either the next tag or the end of the buffer.
 * 
 * tag must not be nullptr. it is the buffer containing NBT tag data.
 * 
 * if end is not nullptr it is used for validation - to ensure the method doesn't overrun the buffer.
 * if stepping over the tag would read past end, the method returns nullptr.
 */
char *nbt_step(char *tag, const char *end) __nbt_nonnull(1);

/**
 * nbt_payload_step returns the byte following the payload,
 * which for valid NBT data is either the next tag, the next payload, or the end of the buffer.
 * 
 * payload must not be nullptr. it is the buffer containing NBT payload data.
 * 
 * type is the type of the payload and must be accurate.
 * it cannot be NBT_END as that implies the pointer has already overrun this tags bounds.
 * 
 * if end is not nullptr it is used for validation - to ensure the method doesn't overrun the buffer.
 * if stepping over the tag would read past end, the method returns nullptr.
 */
char *nbt_payload_step(char *payload, char type, const char *end) __nbt_nonnull(1);



//===============//
// Serialisation //
//===============//

#define NBT_ANY_NUMBER -2
#define NBT_ANY_INTEGER -1


#define NBT_END 0
#define NBT_BYTE 1
#define NBT_SHORT 2
#define NBT_INT 3
#define NBT_LONG 4
#define NBT_FLOAT 5
#define NBT_DOUBLE 6
#define NBT_BYTE_ARRAY 7
#define NBT_STRING 8
#define NBT_LIST 9
#define NBT_COMPOUND 10
#define NBT_INT_ARRAY 11
#define NBT_LONG_ARRAY 12

/** true if the type is a valid nbt tag type. */
#define nbt_type_is_valid(type) (\
    (type) == NBT_END ||\
    (type) == NBT_BYTE ||\
    (type) == NBT_SHORT ||\
    (type) == NBT_INT ||\
    (type) == NBT_LONG ||\
    (type) == NBT_FLOAT ||\
    (type) == NBT_DOUBLE ||\
    (type) == NBT_BYTE_ARRAY ||\
    (type) == NBT_STRING ||\
    (type) == NBT_LIST ||\
    (type) == NBT_COMPOUND ||\
    (type) == NBT_INT_ARRAY ||\
    (type) == NBT_LONG_ARRAY\
)

/** true if the type has a name and payload. */
#define nbt_type_has_payload(type) (\
    (type) == NBT_BYTE ||\
    (type) == NBT_SHORT ||\
    (type) == NBT_INT ||\
    (type) == NBT_LONG ||\
    (type) == NBT_FLOAT ||\
    (type) == NBT_DOUBLE ||\
    (type) == NBT_BYTE_ARRAY ||\
    (type) == NBT_STRING ||\
    (type) == NBT_LIST ||\
    (type) == NBT_COMPOUND ||\
    (type) == NBT_INT_ARRAY ||\
    (type) == NBT_LONG_ARRAY\
)

/** true if the type contains other tags or payloads. */
#define nbt_type_has_children(type) (\
    (type) == NBT_BYTE_ARRAY ||\
    (type) == NBT_STRING ||\
    (type) == NBT_LIST ||\
    (type) == NBT_COMPOUND ||\
    (type) == NBT_INT_ARRAY ||\
    (type) == NBT_LONG_ARRAY\
)

#define nbt_type_is_integer(type) (\
    (type) == NBT_BYTE ||\
    (type) == NBT_SHORT ||\
    (type) == NBT_INT ||\
    (type) == NBT_LONG\
)

#define nbt_type_is_number(type) (\
    nbt_type_is_integer(type) ||\
    (type) == NBT_FLOAT ||\
    (type) == NBT_DOUBLE\
)

/** string constant for the given type. */
#define nbt_type_as_string(type) (\
    (type) == NBT_END ? "NBT_END" :\
    (type) == NBT_BYTE ? "NBT_BYTE" :\
    (type) == NBT_SHORT ? "NBT_SHORT" :\
    (type) == NBT_INT ? "NBT_INT" :\
    (type) == NBT_LONG ? "NBT_LONG" :\
    (type) == NBT_FLOAT ? "NBT_FLOAT" :\
    (type) == NBT_DOUBLE ? "NBT_DOUBLE" :\
    (type) == NBT_BYTE_ARRAY ? "NBT_BYTE_ARRAY" :\
    (type) == NBT_STRING ? "NBT_STRING" :\
    (type) == NBT_LIST ? "NBT_LIST" :\
    (type) == NBT_COMPOUND ? "NBT_COMPOUND" :\
    (type) == NBT_INT_ARRAY ? "NBT_INT_ARRAY" :\
    (type) == NBT_LONG_ARRAY ? "NBT_LONG_ARRAY" :\
    "INVALID TAG TYPE"\
)

/** (3) the type of the tag. tag must not be nullptr. */
static __nbt_nonnull(1)
char nbt_type(const char *tag, const char *end) {
    __nbt_assert(tag != nullptr);
    __nbt_assert(__nbt_has_data(tag, end, 1));
    return tag[0];
}

/** (3) parses the size of the tag name. tag must not be nullptr. returns 0 if tag is malformed. */
static __nbt_nonnull(1)
uint16_t nbt_name_size(const char *tag, const char *end) {
    __nbt_assert(tag != nullptr);
    __nbt_assert(__nbt_has_data(tag, end, 3));
    if (!nbt_type_has_payload(tag[0])) return 0;
    return 
        ((uint16_t)(unsigned char)(tag)[1]<<8) +
        ((uint16_t)(unsigned char)(tag)[2]);
}

/** (3) returns the non-null-terminated name of the tag. tag must not be nullptr. returns "" if tag is malformed. */
static __nbt_nonnull(1)
const char *nbt_name(const char *tag, const char *end) {
    __nbt_assert(tag != nullptr);
    if (!__nbt_has_data(tag, end, 3)) return "";
    if (!__nbt_has_data(tag, end, 3 + nbt_name_size(tag, end))) return "";
    if (!nbt_type_has_payload(tag[0])) return "";
    return tag + 3;
}

/** (3 + name_size) returns the tag's payload. returns nullptr if tag is nullptr, malformed or not of the given type. */
static
char *nbt_payload(char *tag, const char type, const char *end) {
    __nbt_assert(nbt_type_has_payload(type));
    if (!__nbt_has_data(tag, end, 1)) return nullptr;
    if (tag == nullptr || tag[0] != type) return nullptr;

    if (!__nbt_has_data(tag, end, 3)) return nullptr;
    const uint16_t name_size =
        ((uint16_t)(unsigned char)tag[1]<<8) +
        ((uint16_t)(unsigned char)tag[2]);

    if (!__nbt_has_data(tag, end, 3 + name_size)) return nullptr;
    tag += 3 + name_size;

    switch (type){
    case NBT_BYTE: if (!__nbt_has_data(tag, end, 1)) return nullptr; break;
    case NBT_SHORT: if (!__nbt_has_data(tag, end, 2)) return nullptr; break;
    case NBT_INT: if (!__nbt_has_data(tag, end, 4)) return nullptr; break;
    case NBT_LONG: if (!__nbt_has_data(tag, end, 8)) return nullptr; break;
    case NBT_FLOAT: if (!__nbt_has_data(tag, end, 4)) return nullptr; break;
    case NBT_DOUBLE: if (!__nbt_has_data(tag, end, 8)) return nullptr; break;
    case NBT_BYTE_ARRAY: if (!__nbt_has_data(tag, end, 4)) return nullptr; break;
    case NBT_STRING: if (!__nbt_has_data(tag, end, 2)) return nullptr; break;
    case NBT_LIST: if (!__nbt_has_data(tag, end, 5)) return nullptr; break;
    case NBT_INT_ARRAY: if (!__nbt_has_data(tag, end, 4)) return nullptr; break;
    case NBT_LONG_ARRAY: if (!__nbt_has_data(tag, end, 4)) return nullptr; break;
    }

    return tag;
}

/** (1) parses a byte payload. payload must not be nullptr. */
static __nbt_nonnull(1)
int8_t nbt_byte(const char *payload) {
    __nbt_assert(payload != nullptr);
    return payload[0];
}

/** (2) parses a short payload. payload must not be nullptr. */
static __nbt_nonnull(1)
int16_t nbt_short(const char *payload) {
    __nbt_assert(payload != nullptr);
    return 
        ((int16_t)(unsigned char)payload[0]<<8) +
        ((int16_t)(unsigned char)payload[1]);
}

/** (4) parses an int payload. payload must not be nullptr. */
static __nbt_nonnull(1)
int32_t nbt_int(const char *payload) {
    __nbt_assert(payload != nullptr);
    return 
        ((int32_t)(unsigned char)payload[0]<<24) +
        ((int32_t)(unsigned char)payload[1]<<16) +
        ((int32_t)(unsigned char)payload[2]<<8) +
        ((int32_t)(unsigned char)payload[3]);
}

/** (8) parses a long payload. payload must not be nullptr. */
static __nbt_nonnull(1)
int64_t nbt_long(const char *payload) {
    __nbt_assert(payload != nullptr);
    return 
        ((int64_t)(unsigned char)payload[0]<<56) +
        ((int64_t)(unsigned char)payload[1]<<48) +
        ((int64_t)(unsigned char)payload[2]<<40) +
        ((int64_t)(unsigned char)payload[3]<<32) +
        ((int64_t)(unsigned char)payload[4]<<24) +
        ((int64_t)(unsigned char)payload[5]<<16) +
        ((int64_t)(unsigned char)payload[6]<<8) +
        ((int64_t)(unsigned char)payload[7]);
}

/** (4) parses a float payload. payload must not be nullptr. */
static __nbt_nonnull(1)
float nbt_float(const char *payload) {
    __nbt_assert(payload != nullptr);
    union { uint32_t i; float f; } bits;
    bits.i = 
        ((uint32_t)(unsigned char)payload[0]<<24) +
        ((uint32_t)(unsigned char)payload[1]<<16) +
        ((uint32_t)(unsigned char)payload[2]<<8) +
        ((uint32_t)(unsigned char)payload[3]);
    return bits.f;
}

/** (8) parses a double payload. payload must not be nullptr. */
static __nbt_nonnull(1)
double nbt_double(const char *payload) {
    __nbt_assert(payload != nullptr);
    union { uint64_t l; double d; } bits;
    bits.l = 
        ((uint64_t)(unsigned char)payload[0]<<56) +
        ((uint64_t)(unsigned char)payload[1]<<48) +
        ((uint64_t)(unsigned char)payload[2]<<40) +
        ((uint64_t)(unsigned char)payload[3]<<32) +
        ((uint64_t)(unsigned char)payload[4]<<24) +
        ((uint64_t)(unsigned char)payload[5]<<16) +
        ((uint64_t)(unsigned char)payload[6]<<8) +
        ((uint64_t)(unsigned char)payload[7]);
    return bits.d;
}

/** (4) parses the size of the byte array. payload must not be nullptr. */
static __nbt_nonnull(1)
int32_t nbt_byte_array_size(const char *payload) {
    __nbt_assert(payload != nullptr);
    return
        ((int32_t)(unsigned char)(payload)[0]<<24) +
        ((int32_t)(unsigned char)(payload)[1]<<16) +
        ((int32_t)(unsigned char)(payload)[2]<<8) +
        ((int32_t)(unsigned char)(payload)[3]);
}

/** (4) returns the byte array. returns nullptr if payload is nullptr. */
static
char *nbt_byte_array(char *payload) {
    if (payload == nullptr) return nullptr;
    return payload + 4;
}

/** (2) parses the size of the string. payload must not be nullptr. */
static __nbt_nonnull(1)
uint16_t nbt_string_size(const char *payload) {
    __nbt_assert(payload != nullptr);
    return
        ((uint16_t)(unsigned char)payload[0]<<8) +
        ((uint16_t)(unsigned char)payload[1]);
}

/** (2) returns the non null-terminated string. returns nullptr if payload is nullptr. */
static
char *nbt_string(char *payload) {
    if (payload == nullptr) return nullptr;
    return payload + 2;
}

/** (5) parses the element type portion of a list payload. payload must not be nullptr. */
static __nbt_nonnull(1)
char nbt_list_etype(const char *payload) {
    __nbt_assert(payload != nullptr);
    return payload[0];
}

/** (5) parses the size portion of a list payload. payload must not be nullptr. */
static __nbt_nonnull(1)
int32_t nbt_list_size(const char *payload) {
    __nbt_assert(payload != nullptr);
    return
        ((int32_t)(unsigned char)payload[1]<<24) +
        ((int32_t)(unsigned char)payload[2]<<16) +
        ((int32_t)(unsigned char)payload[3]<<8) +
        ((int32_t)(unsigned char)payload[4]);
}

/** (5) returns the first list element. returns nullptr if payload is nullptr. */
static
char *nbt_list_payload(char *payload) {
    if (payload == nullptr) return nullptr;
    return payload + 5;
}

/** (4) parses the size of the int array. payload must not be nullptr. */
static __nbt_nonnull(1)
int32_t nbt_int_array_size(const char *payload) {
    __nbt_assert(payload != nullptr);
    return
        ((int32_t)(unsigned char)payload[0]<<24) +
        ((int32_t)(unsigned char)payload[1]<<16) +
        ((int32_t)(unsigned char)payload[2]<<8) +
        ((int32_t)(unsigned char)payload[3]);
}

/** (4) returns the first int payload in the int array. returns nullptr if payload is nullptr. */
static
char *nbt_int_array_payload(char *payload) {
    if (payload == nullptr) return nullptr;
    return payload + 4;
}

/** (4) parses the size of the long array. payload must not be nullptr. */
static __nbt_nonnull(1)
int32_t nbt_long_array_size(const char *payload) {
    __nbt_assert(payload != nullptr);
    return
        ((int32_t)(unsigned char)payload[0]<<24) +
        ((int32_t)(unsigned char)payload[1]<<16) +
        ((int32_t)(unsigned char)payload[2]<<8) +
        ((int32_t)(unsigned char)payload[3]);
}

/** (4) returns the first long payload in the long array. returns nullptr if payload is nullptr. */
static
char *nbt_long_array_payload(char *payload) {
    if (payload == nullptr) return nullptr;
    return payload + 4;
}

/** returns the value of an integer type tag. the type must be an integer. tag must not be nullptr. */
static __nbt_nonnull(1)
int64_t nbt_autointeger(const char *tag, const char *end) {
    __nbt_assert(tag != nullptr);
    __nbt_assert(__nbt_has_data(tag, end, 3));
    __nbt_assert(nbt_type_is_integer(tag[0]));

    const uint16_t name_size =
        ((uint16_t)(unsigned char)tag[1]<<8) +
        ((uint16_t)(unsigned char)tag[2]);

    switch (tag[0]){
    case NBT_BYTE: return nbt_byte(tag + 3 + name_size);
    case NBT_SHORT: return nbt_short(tag + 3 + name_size);
    case NBT_INT: return nbt_int(tag + 3 + name_size);
    case NBT_LONG: return nbt_long(tag + 3 + name_size);
    default: __builtin_unreachable();
    }
}

/** returns the value of a number type tag. the type must be an number. tag must not be nullptr. */
static __nbt_nonnull(1)
double nbt_autonumber(const char *tag) {
    __nbt_assert(tag != nullptr);
    __nbt_assert(nbt_type_is_number(tag[0]));

    uint16_t name_size =
        ((uint16_t)(unsigned char)tag[1]<<8) +
        ((uint16_t)(unsigned char)tag[2]);

    switch (tag[0]){
    case NBT_BYTE: return nbt_byte(tag + 3 + name_size);
    case NBT_SHORT: return nbt_short(tag + 3 + name_size);
    case NBT_INT: return nbt_int(tag + 3 + name_size);
    case NBT_LONG: return nbt_long(tag + 3 + name_size);
    case NBT_FLOAT: return nbt_float(tag + 3 + name_size);
    case NBT_DOUBLE: return nbt_double(tag + 3 + name_size);
    default: __builtin_unreachable();
    }
}

/** 
 * ((i / floor(64 / bits)) * 8 + 8)
 * returns the packed integer at the given index in the packed array. 
 * data must not be nullptr
 */
static
uint64_t nbt_packed_array_elem(char *payload, const int i, const int bits) {
    __nbt_assert(payload != nullptr);

    const auto size = nbt_long_array_size(payload);
    payload = nbt_long_array_payload(payload);

    const int c = 64 / bits;

    __nbt_assert((i / c) < size);

    payload += (i / c) * 8;
    const uint64_t n =
        ((uint64_t)(unsigned char)payload[0] << 56) +
        ((uint64_t)(unsigned char)payload[1] << 48) +
        ((uint64_t)(unsigned char)payload[2] << 40) +
        ((uint64_t)(unsigned char)payload[3] << 32) +
        ((uint64_t)(unsigned char)payload[4] << 24) +
        ((uint64_t)(unsigned char)payload[5] << 16) +
        ((uint64_t)(unsigned char)payload[6] << 8) +
        ((uint64_t)(unsigned char)payload[7]);

    return n >> i % c * bits & ((~0ULL) >> (64 - bits));
}



//=================//
// Syntactic Sugar //
//=================//

/**
 * finds the child tag in the compound payload that has the given name.
 * 
 * payload is a buffer containing a compound payload.
 * end is the byte after the end of the buffer.
 * 
 * the varidic arguments are a null-termianted repeating set of
 * char *name, size_t name_size, char type, char **dest
 * 
 * name is the name to search for.
 * name_size is the size of name.
 * type is the type of the tag, or one of the special 'any' types.
 * dest is assigned to the found tag's payload if found and of the correct type.
 * 
 * if payload is nullptr or malformed it returns nullptr.
 * it returns the byte after the payload's end.
 * 
 * if type is NBT_ANY_NUMBER is used, dest must be int64_t*.
 * if 
 * 
 * Example:
```C
struct anvil_chunk chunk = anvil_chunk_decompress(chunk_ctx, &region, x, z);

char *data_version = nullptr, *status = nullptr;
char *end = nbt_named(chunk.data, chunk.data + chunk.data_size,
    "DataVersion", strlen("DataVersion"), NBT_INT, &data_version,
    "Status", strlen("Status"), NBT_STRING, &status,
    nullptr
);

if (end == nullptr) {
    printf("corrupted NBT data.\n");
} else if (status == nullptr) {
    printf("no tag with name 'Status' and type NBT_STRING found.\n");
} else {
    printf(
        "chunk status: %.*s\n", 
        nbt_string_size(status),
        nbt_string(status)
    );
}

```
 */
char *nbt_named(char *payload, const char *end,
    const char *name, size_t name_size, int type, void *dest,
    ...
);

/**
 * @}
 */