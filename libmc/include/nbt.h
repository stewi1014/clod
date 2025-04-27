/**
 * nbt.h
 * 
 * very simple library with no faffing about.
 * 
 * Contains tools helpful for parsing NBT data.
 * Notably it doesn't include any intermediary data structure that NBT data is parsed into;
 * it doesn't attempt to abstract away the parsing proccess, not really.
 * Instead, it's designed to provide tools for making concice parsers.
 * 
 * The main method is `nbt_payload_step` and its macro partner for tags: `nbt_step`.
 * They simply return a pointer to the byte following the given payload or tag.
 * When searching for 
 */

#pragma once
#define __nbt_null ((void*)0)

/**
 * nbt_step returns the pointer to the byte following the tag.
 * typically this is the next tag.
 * 
 * if tag is NULL it returns NULL.
 * if the tag is malformed it returns NULL.
 */
#define nbt_step(tag) (\
    (tag) == __nbt_null || !nbt_type_is_valid((tag)[0]) ? __nbt_null :\
    (tag)[0] == NBT_END ? (tag) + 1 :\
    nbt_payload_step((tag)[0], (tag) + 3 + nbt_name_size(tag))\
)

/**
 * nbt_payload_step returns the pointer to the byte following the payload.
 * typically this is the next tag.
 * 
 * if payload is NULL it returns NULL.
 * if the tag is malformed or has no payload it returns NULL.
 */
char *nbt_payload_step(char type, char *payload);

/** true if the type has a name and payload */
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

/** true if the type is a valid nbt tag type */
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

/** the type of the tag. 0 if tag is NULL, or if the tag is invalid */
#define nbt_type(tag) ((tag) == __nbt_null ? 0 : (tag)[0])

/**
 * the size of the tag's name.
 * if tag is NULL, the tag is malformed, or the tag doesn't have a name it is 0.
 */
#define nbt_name_size(tag) ((tag) == __nbt_null || !nbt_type_has_payload((tag)[0]) ? 0 :\
    ((unsigned short)(unsigned char)(tag)[1]<<8) +\
    ((unsigned short)(unsigned char)(tag)[2])\
)

/**
 * the name of the tag.
 * if tag is NULL, the tag is malformed, or the tag doesn't have a name it is an empty string.
 * 
 * it is *not* null-terminated. use nbt_name_size.
 */
#define nbt_name(tag) (\
    (tag) == __nbt_null || \
    !nbt_type_has_payload((tag)[0]) ||\
    (((unsigned char)(tag)[1]<<8) + ((unsigned char)(tag)[2])) == 0\
        ? "" : &(tag)[3]\
)

/**
 * the payload of the tag.
 * if the tag is NULL, the tag is malformed, or the tag isn't the specified type it is NULL.
 */
#define nbt_payload(tag, type) (\
    (tag) == __nbt_null || \
    !nbt_type_has_payload(type) ||\
    (tag)[0] != (type)\
    ? __nbt_null : (tag) + 3 + ((unsigned char)(tag)[1]<<8) + ((unsigned char)(tag)[2])\
)



//==============//
// Byte Payload //
//==============//

/** byte nbt type. */
#define NBT_BYTE 1

/** value of the byte payload. 0 if payload is NULL. */
#define nbt_byte(payload) ((payload) == __nbt_null ? 0 :\
    (payload)[0]\
)



//===============//
// Short Payload //
//===============//

/** short nbt type. */
#define NBT_SHORT 2

/** value of the short payload. 0 if payload is NULL. */
#define nbt_short(payload) ((payload) == __nbt_null ? 0 :\
    ((short)(unsigned char)(payload)[0]<<8) +\
    ((short)(unsigned char)(payload)[1])\
)



//=============//
// Int Payload //
//=============//

/** int nbt type. */
#define NBT_INT 3

/** value of the int payload. 0 if payload is NULL. */
#define nbt_int(payload) ((payload) == __nbt_null ? 0 :\
    ((int)(unsigned char)(payload)[0]<<24) +\
    ((int)(unsigned char)(payload)[1]<<16) +\
    ((int)(unsigned char)(payload)[2]<<8) +\
    ((int)(unsigned char)(payload)[3])\
)



//==============//
// Long Payload //
//==============//

/** long nbt type. */
#define NBT_LONG 4

/** value of the long payload. 0 if payload is NULL. */
#define nbt_long(payload) ((payload) == __nbt_null ? 0 :\
    ((long)(unsigned char)(payload)[0]<<56) +\
    ((long)(unsigned char)(payload)[1]<<48) +\
    ((long)(unsigned char)(payload)[2]<<40) +\
    ((long)(unsigned char)(payload)[3]<<32) +\
    ((long)(unsigned char)(payload)[4]<<24) +\
    ((long)(unsigned char)(payload)[5]<<16) +\
    ((long)(unsigned char)(payload)[6]<<8) +\
    ((long)(unsigned char)(payload)[7])\
)



//===============//
// Float Payload //
//===============//

/** float nbt type. */
#define NBT_FLOAT 5

/** value of the float payload. 0 if payload is NULL. */
#define nbt_float(payload) ((payload) == __nbt_null ? (float)0.0 : {\
    union {unsigned int b;float f;} u = {.b =\
    ((unsigned int)(unsigned char)(payload)[0]<<24) +\
    ((unsigned int)(unsigned char)(payload)[1]<<16) +\
    ((unsigned int)(unsigned char)(payload)[2]<<8) +\
    ((unsigned int)(unsigned char)(payload)[3])\
}; u.f })



//================//
// Double Payload //
//================//

/** double nbt type. */
#define NBT_DOUBLE 6

/** value of the double payload. 0 if payload is NULL. */
#define nbt_double(payload) ((payload) == __nbt_null ? (double)0.0 : {\
    union {unsigned long b;double d;} u = {.b =\
    ((unsigned long)(unsigned char)(payload)[0]<<56) +\
    ((unsigned long)(unsigned char)(payload)[1]<<48) +\
    ((unsigned long)(unsigned char)(payload)[2]<<40) +\
    ((unsigned long)(unsigned char)(payload)[3]<<32) +\
    ((unsigned long)(unsigned char)(payload)[4]<<24) +\
    ((unsigned long)(unsigned char)(payload)[5]<<16) +\
    ((unsigned long)(unsigned char)(payload)[6]<<8) +\
    ((unsigned long)(unsigned char)(payload)[7])\
}; u.d })



//====================//
// Byte Array Payload //
//====================//

/** byte array nbt type. */
#define NBT_BYTE_ARRAY 7

/** first byte in the byte array. NULL if payload is NULL. */
#define nbt_byte_array(payload) ((payload) == __nbt_null ? NULL :\
    &(payload)[4]\
)

/** size of the byte array. 0 if payload is NULL. */
#define nbt_byte_array_size(payload) ((payload) == __nbt_null ? 0 :\
    ((int)(unsigned char)(payload)[0]<<24) +\
    ((int)(unsigned char)(payload)[1]<<16) +\
    ((int)(unsigned char)(payload)[2]<<8) +\
    ((int)(unsigned char)(payload)[3])\
)




//================//
// String Payload //
//================//

/** string nbt type. */
#define NBT_STRING 8

/** 
 * first character in the string.
 * 
 * it is not null-terminated. use nbt_string_size.
 */
#define nbt_string(payload) ((payload) == __nbt_null ? __nbt_null :\
    &(payload)[2]\
)

/** size of the string. 0 if payload is NULL. */
#define nbt_string_size(payload) ((payload) == __nbt_null ? 0 :\
    ((unsigned short)(unsigned char)(payload)[0]<<8) +\
    ((unsigned short)(unsigned char)(payload)[1])\
)

//==============//
// List Payload //
//==============//

/** list nbt type. */
#define NBT_LIST 9

/** first payload in the list. NULL if payload is NULL. */
#define nbt_list(payload) ((payload) == __nbt_null ? __nbt_null :\
    &(payload)[5]\
)

/** size in payloads of the list. 0 if payload is NULL. */
#define nbt_list_size(payload) ((payload) == __nbt_null ? 0 :\
    ((int)(unsigned char)(payload)[1]<<24) +\
    ((int)(unsigned char)(payload)[2]<<16) +\
    ((int)(unsigned char)(payload)[3]<<8) +\
    ((int)(unsigned char)(payload)[4])\
)

/** type of payload that the list holds. 0 if payload is NULL. */
#define nbt_list_etype(payload) ((payload) == __nbt_null ? 0 :\
    (payload)[0]\
)

/** 
 * iterates over a list.
 * 
 * It is supposed to be used like
 * 
 *  char *elem;
 *  for (int i = 0; elem = nbt_list_iter(list_payload, elem, i, NBT_STRING); i++) {
 *      printf(
 *          "index %d, 
 *          string value \"%.*s\"\n", 
 *          nbt_string_size(elem), 
 *          nbt_string(elem)
 *      );
 *  }
 * 
 * it is the value of the next list element.
 * if payload is NULL it is NULL.
 * if the list does not contain payloads of type etype it is NULL.
 * if i >= list size it is NULL.
 * if i == 0 it is the address of the first element.
 * if elem == NULL && i != 0 it is NULL.
 */
#define nbt_list_iter(payload, elem, i, etype) (\
    (payload) == __nbt_null || (payload)[0] != etype ? __nbt_null :\
    (i) >= (\
        ((int)(unsigned char)(payload)[1]<<24) +\
        ((int)(unsigned char)(payload)[2]<<16) +\
        ((int)(unsigned char)(payload)[3]<<8) +\
        ((int)(unsigned char)(payload)[4])\
    ) ? NULL :\
    (i) == 0 ? &(payload)[5] :\
    (elem) == NULL ? NULL : nbt_payload_step(etype, elem)\
)

//==================//
// Compound Payload //
//==================//

/** compound nbt type. */
#define NBT_COMPOUND 10

/** end nbt type. */
#define NBT_END 0

/** true if the payload is a */
#define nbt_compound_end(payload)


//===================//
// Int Array Payload //
//===================//

/** int array nbt type. */
#define NBT_INT_ARRAY 11

/** first payload in the int array. NULL if payload is NULL. */
#define nbt_int_array(payload) ((payload) == __nbt_null ? __nbt_null :\
    &(payload)[4]\
)

/** size in ints of an int array. 0 if payload is NULL. */
#define nbt_int_array_size(payload) ((payload) == __nbt_null ? 0 :\
    ((int)(unsigned char)(payload)[0]<<24) +\
    ((int)(unsigned char)(payload)[1]<<16) +\
    ((int)(unsigned char)(payload)[2]<<8) +\
    ((int)(unsigned char)(payload)[3])\
)



//====================//
// Long Array Payload //
//====================//

/** long array nbt type. */
#define NBT_LONG_ARRAY 12

/** first payload in the long array. NULL if payload is NULL. */
#define nbt_long_array(payload) ((payload) == __nbt_null ? __nbt_null :\
    &(payload)[4]\
)

/** size in longs of the long array. 0 if payload is NULL. */
#define nbt_long_array_size(payload) ((payload) == __nbt_null ? 0 :\
    ((int)(unsigned char)(payload)[0]<<24) +\
    ((int)(unsigned char)(payload)[1]<<16) +\
    ((int)(unsigned char)(payload)[2]<<8) +\
    ((int)(unsigned char)(payload)[3])\
)
