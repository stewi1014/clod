/**
 * nbt.h
 * 
 * Aims to make it straightforward to operate on serialised NBT data, but does not offer any intermediate representation of NBT data.
 * 
 * Methods operate directly on the serialised data, which makes it fast as all get-out for parsing and making changes to serialised data.
 * However some use cases do not need changes to be serialised,
 * and can benifit from storing changes in an intermediate data format - this library is not for those use cases.
 * 
 * This approach also comes with an ergonomic downside;
 * Nuances of the NBT format are not abstracted away by this library, and using it can be verbose.
 * So, if you're going to use it, make sure you're aware of how the NBT format works.
 */

#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __GNUC__
#define __nbt_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))
#define __nbt_nonnull_size(str, size) __attribute__((nonnull_if_nonzero(str, size)))
#else
#define __nbt_nonnull(...)
#define __nbt_nonnull_size(str, size)
#endif

// disable asserts in release builds.
#ifndef NDEBUG
#define __nbt_assert(expr) assert(expr)
#else
#define __nbt_assert(expr)
#endif

#define __nbt_has_data(ptr, end, size) ((size) >= 0 && ((char*)(end) == NULL || (ptr) <= ((char*)(end)-(size))))

//=======================//
// Primary Functionality //
//=======================//

/**
 * nbt_step returns the byte following the tag,
 * which for valid NBT data is either the next tag or the end of the buffer.
 * 
 * tag must not be NULL. it is the buffer containing NBT tag data.
 * 
 * if end is not NULL it is used for validation - to ensure the method doesn't overrun the buffer.
 * if steping over the tag would read past end, the method returns NULL.
 */
char *nbt_step(char *tag, char *end) __nbt_nonnull(1);

/**
 * nbt_payload_step returns the byte following the payload,
 * which for valid NBT data is either the next tag, the next payload, or the end of the buffer.
 * 
 * payload must not be NULL. it is the buffer containing NBT payload data.
 * 
 * type is the type of the payload and must be accurate.
 * it cannot be NBT_END as that implies the pointer has already overran this tags bounds.
 * 
 * if end is not NULL it is used for validation - to ensure the method doesn't overrun the buffer.
 * if steping over the tag would read past end, the method returns NULL.
 */
char *nbt_payload_step(char *payload, char type, char *end) __nbt_nonnull(1);

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

/** (3) the type of the tag. tag must not be NULL. */
static inline __nbt_nonnull(1)
char nbt_type(char *tag) {
    __nbt_assert(tag != NULL);
    return tag[0];
}

/** sets the type of the tag. tag and end must not be NULL. type must be valid. returns new end. */
char *nbt_type_set(char *tag, char type, char *end) __nbt_nonnull(1, 3);

/** (3) parses the size of the tag name. tag must not be NULL. returns 0 if tag is malformed. */
static inline __nbt_nonnull(1)
uint16_t nbt_name_size(char *tag) {
    __nbt_assert(tag != NULL);
    if (!nbt_type_has_payload(tag[0])) return 0;
    return 
        ((uint16_t)(unsigned char)(tag)[1]<<8) +
        ((uint16_t)(unsigned char)(tag)[2]);
}

/** (3) returns the non-null-terminated name of the tag. tag must not be NULL. returns NULL if tag is malformed. */
static inline __nbt_nonnull(1)
char *nbt_name(char *tag) {
    __nbt_assert(tag != NULL);
    if (!nbt_type_has_payload(tag[0])) return NULL;
    return tag + 3;
}

/** sets the name of the tag. tag, end and name must not be null. name must be null terminated. returns new end. */
char *nbt_name_set(char *tag, char *end, char *name) __nbt_nonnull(1, 2, 3);

/** sets the name of the tag. tag, end and name must not be null. name_size must be accurate. returns new end. */
char *nbt_name_setn(char *tag, char *end, char *name, uint16_t name_size) __nbt_nonnull(1, 2, 3);

/** (3 + name_size) returns the tag's payload. returns NULL if tag is NULL or not of the given type. */
static inline
char *nbt_payload(char *tag, char type) {
    __nbt_assert(nbt_type_has_payload(type));
    if (tag == NULL || tag[0] != type) return NULL;

    uint16_t name_size =
        ((uint16_t)(unsigned char)tag[1]<<8) +
        ((uint16_t)(unsigned char)tag[2]);

    return tag + 3 + name_size;
}

/** (1) parses a byte payload. payload must not be NULL. */
static inline __nbt_nonnull(1)
int8_t nbt_byte(char *payload) {
    __nbt_assert(payload != NULL);
    return payload[0];
}

/** (2) parses a short payload. payload must not be NULL. */
static inline __nbt_nonnull(1)
int16_t nbt_short(char *payload) {
    __nbt_assert(payload != NULL);
    return 
        ((int16_t)(unsigned char)payload[0]<<8) +
        ((int16_t)(unsigned char)payload[1]);
}

/** (4) parses an int payload. payload must not be NULL. */
static inline __nbt_nonnull(1)
int32_t nbt_int(char *payload) {
    __nbt_assert(payload != NULL);
    return 
        ((int32_t)(unsigned char)payload[0]<<24) +
        ((int32_t)(unsigned char)payload[1]<<16) +
        ((int32_t)(unsigned char)payload[2]<<8) +
        ((int32_t)(unsigned char)payload[3]);
}

/** (8) parses a long payload. payload must not be NULL. */
static inline __nbt_nonnull(1)
int64_t nbt_long(char *payload) {
    __nbt_assert(payload != NULL);
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

/** (4) parses a float payload. payload must not be NULL. */
static inline __nbt_nonnull(1)
float nbt_float(char *payload) {
    __nbt_assert(payload != NULL);
    union { uint32_t i; float f; } bits;
    bits.i = 
        ((uint32_t)(unsigned char)payload[0]<<24) +
        ((uint32_t)(unsigned char)payload[1]<<16) +
        ((uint32_t)(unsigned char)payload[2]<<8) +
        ((uint32_t)(unsigned char)payload[3]);
    return bits.f;
}

/** (8) parses a double payload. payload must not be NULL. */
static inline __nbt_nonnull(1)
double nbt_double(char *payload) {
    __nbt_assert(payload != NULL);
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

/** (4) parses the size of the byte array. payload must not be NULL. */
static inline __nbt_nonnull(1)
int32_t nbt_byte_array_size(char *payload) {
    __nbt_assert(payload != NULL);
    return
        ((int32_t)(unsigned char)(payload)[0]<<24) +
        ((int32_t)(unsigned char)(payload)[1]<<16) +
        ((int32_t)(unsigned char)(payload)[2]<<8) +
        ((int32_t)(unsigned char)(payload)[3]);
}

/** (4) returns the byte array. returns NULL if payload is NULL. */
static inline
char *nbt_byte_array(char *payload) {
    if (payload == NULL) return NULL;
    return payload + 4;
}

/** (2) parses the size of the string. payload must not be NULL. */
static inline __nbt_nonnull(1)
uint16_t nbt_string_size(char *payload) {
    __nbt_assert(payload != NULL);
    return
        ((uint16_t)(unsigned char)payload[0]<<8) +
        ((uint16_t)(unsigned char)payload[1]);
}

/** (2) returns the non null-terminated string. returns NULL if payload is NULL. */
static inline
char *nbt_string(char *payload) {
    if (payload == NULL) return NULL;
    return payload + 2;
}

/** (5) parses the element type portion of a list payload. payload must not be NULL. */
static inline __nbt_nonnull(1)
char nbt_list_etype(char *payload) {
    __nbt_assert(payload != NULL);
    return payload[0];
}

/** (5) parses the size portion of a list payload. payload must not be NULL. */
static inline __nbt_nonnull(1)
int32_t nbt_list_size(char *payload) {
    __nbt_assert(payload != NULL);
    return
        ((int32_t)(unsigned char)payload[1]<<24) +
        ((int32_t)(unsigned char)payload[2]<<16) +
        ((int32_t)(unsigned char)payload[3]<<8) +
        ((int32_t)(unsigned char)payload[4]);
}

/** (5) returns the first list element. returns NULL if payload is NULL. */
static inline
char *nbt_list_payload(char *payload) {
    if (payload == NULL) return NULL;
    return payload + 5;
}

/** (4) parses the size of the int array. payload must not be NULL. */
static inline __nbt_nonnull(1)
int32_t nbt_int_array_size(char *payload) {
    __nbt_assert(payload != NULL);
    return
        ((int32_t)(unsigned char)payload[0]<<24) +
        ((int32_t)(unsigned char)payload[1]<<16) +
        ((int32_t)(unsigned char)payload[2]<<8) +
        ((int32_t)(unsigned char)payload[3]);
}

/** (4) returns the first int payload in the int array. returns NULL if payload is NULL. */
static inline
char *nbt_int_array_payload(char *payload) {
    if (payload == NULL) return NULL;
    return payload + 4;
}

/** (4) parses the size of the long array. payload must not be NULL. */
static inline __nbt_nonnull(1)
int32_t nbt_long_array_size(char *payload) {
    __nbt_assert(payload != NULL);
    return
        ((int32_t)(unsigned char)payload[0]<<24) +
        ((int32_t)(unsigned char)payload[1]<<16) +
        ((int32_t)(unsigned char)payload[2]<<8) +
        ((int32_t)(unsigned char)payload[3]);
}

/** (4) returns the first long payload in the long array. returns NULL if payload is NULL. */
static inline
char *nbt_long_array_payload(char *payload) {
    if (payload == NULL) return NULL;
    return payload + 4;
}

//=================//
// Syntactic Sugar //
//=================//

/**
 * finds the child tag in the compound tag that has the given name.
 * 
 * compound_tag is a buffer containing compound nbt tag data.
 * end is the byte after the end of the buffer.
 * name is the name to search for. name must not be NULL, and must be null-terminated.
 * 
 * if tag is NULL it returns NULL.
 * if tag is malformed or not NBT_COMPOUND it returns NULL.
 * if no tag is found it returns an NBT_EMPTY tag.
 */
char *nbt_named(char *compound_tag, char *end, char *name) __nbt_nonnull(3);

/**
 * finds the child tag in the compound tag that has the given name.
 * 
 * compound_tag is a buffer containing compound nbt tag data.
 * end is the byte after the end of the buffer.
 * name is the name to search for. name must not be NULL.
 * name_size is the size of name.
 * 
 * if tag is NULL it returns NULL.
 * if tag is malformed or not NBT_COMPOUND it returns NULL.
 * if no tag is found it returns an NBT_EMPTY tag.
 */
char *nbt_nnamed(char *compound_tag, char *end, char *name, size_t name_size) __nbt_nonnull(3);

/**
 * iterates over elements of a list payload,
 * assigning elem to each element and running the code block.
 * 
 * it steps through each element on its own, so if you're also stepping
 * through elements in the code block - you're doing it twice. *wasteful*
 * 
 * "returns" the byte after the payload, or NULL if payload is NULL or malformed.
 * 
 * Example:
 *  char *sections = nbt_named(chunk_data, end, "sections");
 *  char *section, *tag;
 *  nbt_list_foreach(nbt_payload(sections, NBT_LIST), end, section, 
 *      nbt_compound_foreach(nbt_payload(section, NBT_COMPOUND), end, tag, {
 *          printf("%.*s(%s)\n", nbt_name_size(tag), nbt_name(tag), nbt_type_as_string(nbt_type(tag)));
 *      })
 *  );
 * 
 */
#define nbt_list_foreach(payload, end, elem, code) (\
    (payload) == NULL || !__nbt_has_data(payload, end, 5) ? NULL : ({\
        (elem) = nbt_list_payload(payload);\
        for (int32_t __nbt_##elem##_i = 0; (elem) != NULL && __nbt_##elem##_i < nbt_list_size(payload); __nbt_##elem##_i++) {\
            (code);\
            (elem) = nbt_payload_step(elem, nbt_list_etype(payload), end);\
        };\
        (elem);\
    })\
)

/**
 * iterates over elements of a compound payload,
 * assigning elem to each element and running the code block.
 * 
 * it steps through each element on its own, so if you're also stepping
 * through elements in the code block - you're doing it twice. *wasteful*
 * 
 * "returns" the byte after the payload, or NULL if payload is NULL or malformed.
 * 
 * Example:
 *  char *sections = nbt_named(chunk_data, end, "sections");
 *  char *section, *tag;
 *  nbt_list_foreach(nbt_payload(sections, NBT_LIST), end, section, 
 *      nbt_compound_foreach(nbt_payload(section, NBT_COMPOUND), end, tag, {
 *          printf("%.*s(%s)\n", nbt_name_size(tag), nbt_name(tag), nbt_type_as_string(nbt_type(tag)));
 *      })
 *  );
 * 
 */
#define nbt_compound_foreach(payload, end, elem, code) (\
    (payload) == NULL || !__nbt_has_data(payload, end, 1) ? NULL : ({\
        (elem) = (payload);\
        while (\
            (elem) != NULL &&\
            __nbt_has_data(elem, end, 1) &&\
            (elem)[0] != NBT_END &&\
            nbt_type_is_valid((elem)[0]) &&\
            __nbt_has_data(elem, end, 3)\
        ) {\
            (code);\
            (elem) = nbt_step(elem, end);\
        }\
        (elem) != NULL && \
        __nbt_has_data(elem, end, 1) && \
        (elem)[0] == NBT_END \
            ? (elem) += 1 : NULL;\
    })\
)

/**
 * iterates over ints in the array.
 */
#define nbt_int_array_foreach(payload, end, elem, code) (\
    (payload) == NULL || !__nbt_has_data(payload, end, 4) ? NULL : ({\
        (elem) = nbt_int_array(payload);\
        !__nbt_has_data(elem, end, 4 * nbt_int_array_size(payload)) ? NULL : ({\
            for (int32_t __nbt_##elem##_i = 0; __nbt_##elem##_i < nbt_int_array_size(payload); __nbt_##elem##_i++, (elem) += 4) (code);\
            (elem);\
        });\
    })\
)

/**
 * iterates over longs in the array.
 */
#define nbt_long_array_foreach(payload, end, elem, code) (\
    (payload) == NULL || !__nbt_has_data(payload, end, 4) ? NULL : ({\
        (elem) = nbt_long_array(payload);\
        !__nbt_has_data(elem, end, 8 * nbt_long_array_size(payload)) ? NULL : ({\
            for (int32_t __nbt_##elem##_i = 0; __nbt_##elem##_i < nbt_long_array_size(payload); __nbt_##elem##_i++, (elem) += 8) (code);\
            (elem);\
        });\
    })\
)
