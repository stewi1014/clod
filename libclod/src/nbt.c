#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "nbt.h"

static inline char *nbt_memshift(char *ptr, char *end, size_t old_size, size_t new_size) {
    __nbt_assert(ptr != NULL);
    __nbt_assert(end != NULL);
    __nbt_assert(end >= ptr);

    // if old_size is larger than the apparant size of the entire buffer,
    // I think that means something must be wrong (old_size obviously, but where is it coming from...)
    // this may mean that growing into a buffer that has no existing NBT data might need more care.
    // we're supposed to be able to grow end as much as we need here (user should ensure buffer is large enough for the data they write)
    // so we *could* just return the new valid length in this case, 
    // but I'd rather be anal about buffer sizes and find mistakes
    // than silently fix them some of the time.
    __nbt_assert(end - ptr >= old_size);

    memmove(ptr + new_size, ptr + old_size, end - ptr - old_size);
    return end + new_size - old_size;
}



//=======================//
// Primary Functionality //
//=======================//

char *nbt_step(char *tag, char *end) {
    __nbt_assert(tag != NULL);
    if (!__nbt_has_data(tag, end, 1)) return NULL;
    char type = tag[0];

    if (!nbt_type_is_valid(type)) return NULL;
    if (type == NBT_END) return tag + 1;

    if (!__nbt_has_data(tag, end, 3)) return NULL;
    uint16_t name_size = nbt_name_size(tag, end);

    if (!__nbt_has_data(tag, end, 3 + (size_t)name_size)) return NULL;
    return nbt_payload_step(tag + 3 + (size_t)name_size, type, end);
}

char *nbt_payload_step(char *payload, char type, char *end) {
    __nbt_assert(payload != NULL && nbt_type_has_payload(type));
    if (!__nbt_has_data(payload, end, 1)) return NULL;

    switch (type) {
    case NBT_BYTE: return __nbt_has_data(payload, end, 1) ? payload + 1 : NULL;
    case NBT_SHORT: return __nbt_has_data(payload, end, 2) ? payload + 2 : NULL;
    case NBT_INT: return __nbt_has_data(payload, end, 4) ? payload + 4 : NULL;
    case NBT_LONG: return __nbt_has_data(payload, end, 8) ? payload + 8 : NULL;
    case NBT_FLOAT: return __nbt_has_data(payload, end, 4) ? payload + 4 : NULL;
    case NBT_DOUBLE: return __nbt_has_data(payload, end, 8) ? payload + 8 : NULL;
    case NBT_BYTE_ARRAY: {
        if (!__nbt_has_data(payload, end, 4)) return NULL;
        int32_t size = nbt_byte_array_size(payload);

        if (!__nbt_has_data(payload, end, 4 + (ptrdiff_t)size)) return NULL;
        return payload + 4 + (ptrdiff_t)size;
    }

    case NBT_STRING: {
        if (!__nbt_has_data(payload, end, 2)) return NULL;
        uint16_t size = nbt_string_size(payload);

        if (!__nbt_has_data(payload, end, 2 + (size_t)size)) return NULL;
        return payload + 2 + (size_t)size;
    }

    case NBT_LIST: {
        if (!__nbt_has_data(payload, end, 5)) return NULL;
        char etype = nbt_list_etype(payload);
        int32_t size = nbt_list_size(payload);
        payload = nbt_list_payload(payload);
        switch (etype) {
        case NBT_END: return __nbt_has_data(payload, end, 0) ? payload : NULL;
        case NBT_BYTE: return __nbt_has_data(payload, end, (ptrdiff_t)size) ? (payload + (ptrdiff_t)size) : NULL;
        case NBT_SHORT: return __nbt_has_data(payload, end, (ptrdiff_t)size * 2) ? (payload + (ptrdiff_t)size * 2) : NULL;
        case NBT_INT: return __nbt_has_data(payload, end, (ptrdiff_t)size * 4) ? (payload + (ptrdiff_t)size * 4) : NULL;
        case NBT_LONG: return __nbt_has_data(payload, end, (ptrdiff_t)size * 8) ? (payload + (ptrdiff_t)size * 8) : NULL;
        case NBT_FLOAT: return __nbt_has_data(payload, end, (ptrdiff_t)size * 4) ? (payload + (ptrdiff_t)size * 4) : NULL;
        case NBT_DOUBLE: return __nbt_has_data(payload, end, (ptrdiff_t)size * 8) ? (payload + (ptrdiff_t)size * 8) : NULL;
        case NBT_BYTE_ARRAY:
        case NBT_STRING:
        case NBT_LIST:
        case NBT_COMPOUND:
        case NBT_INT_ARRAY:
        case NBT_LONG_ARRAY: {
            for (int32_t i=0; i<size && payload != NULL; i++)
                payload = nbt_payload_step(payload, etype, end);
            return payload;
        }
        default: return NULL;
        }
    }

    case NBT_COMPOUND: {
        while (payload != NULL && payload[0] != NBT_END)
            payload = nbt_step(payload, end); 
        return payload != NULL && __nbt_has_data(payload, end, 1) ? payload+1 : NULL;
    }

    case NBT_INT_ARRAY: {
        if (!__nbt_has_data(payload, end, 4)) return NULL;
        int32_t size = nbt_int_array_size(payload);
        if (!__nbt_has_data(payload, end, 4 + 4 * (ptrdiff_t)size)) return NULL;
        return payload + 4 + 4 * (ptrdiff_t)size;
    }

    case NBT_LONG_ARRAY: {
        if (!__nbt_has_data(payload, end, 4)) return NULL;
        int32_t size = nbt_long_array_size(payload);
        if (!__nbt_has_data(payload, end, 4 + 8 * (ptrdiff_t)size)) return NULL;
        return payload + 4 + 8 * (ptrdiff_t)size;
    }

    default: return NULL;
    }
}



//===============//
// Serialisation //
//===============//

char *nbt_type_set(char *tag, char type, char *end) {
    __nbt_assert(tag != NULL);
    __nbt_assert(end != NULL);
    __nbt_assert(end >= tag);
    __nbt_assert(nbt_type_is_valid(type));

    if (end == tag) end = tag+1;
    tag[0] = type;
    return end;
}

char *nbt_name_set(char *tag, char *name, char *end) {
    __nbt_assert(name != NULL);
    return nbt_name_setn(tag, end, name, strlen(name));
}

char *nbt_name_setn(char *tag, char *end, char *name, uint16_t new_size){
    __nbt_assert(tag != NULL);
    __nbt_assert(end != NULL);
    __nbt_assert(end >= tag);
    __nbt_assert(name != NULL);

    uint16_t old_size = nbt_name_size(tag, end);
    end = nbt_memshift(tag + 3, end, old_size, new_size);
    memcpy(tag + 3, name, new_size);
    
    tag[1] = (unsigned char)new_size>>8;
    tag[2] = (unsigned char)new_size;
    return end;
}



//=================//
// Syntactic Sugar //
//=================//

char *nbt_named(char *payload, char *end, 
    char *name, size_t name_size, int type, void *dest,
    ...
) {
    if (payload == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, dest);

    while (payload != NULL) {
        if (!__nbt_has_data(payload, end, 1)) {
            return NULL;
        }

        char tag_type = payload[0];
        if (tag_type == NBT_END) {
            return payload + 1;
        }

        if (!nbt_type_is_valid(tag_type) || !__nbt_has_data(payload, end, 3)) {
            return NULL;
        }

        uint16_t tag_name_size = nbt_name_size(payload, end);

        if (!__nbt_has_data(payload, end, 3 + tag_name_size)) {
            return NULL;
        }

        char *va_name = name;
        size_t va_name_size = name_size;
        char va_type = type;
        void *va_dest = dest;
        va_list iter;
        va_copy(iter, va);

    next:
        if (tag_name_size == va_name_size && 0 == strncmp(nbt_name(payload, end), va_name, va_name_size)) {
            if (va_type == NBT_ANY_INTEGER && nbt_type_is_integer(tag_type)) {
                switch (tag_type) {
                case NBT_BYTE: *((int64_t*)va_dest) = nbt_byte(payload + 3 + tag_name_size); break;
                case NBT_SHORT: *((int64_t*)va_dest) = nbt_short(payload + 3 + tag_name_size); break;
                case NBT_INT: *((int64_t*)va_dest) = nbt_int(payload + 3 + tag_name_size); break;
                case NBT_LONG: *((int64_t*)va_dest) = nbt_long(payload + 3 + tag_name_size); break;
                }
            }

            else if (va_type == NBT_ANY_NUMBER && nbt_type_is_number(tag_type)) {
                switch (tag_type) {
                case NBT_BYTE: *((double*)va_dest) = nbt_byte(payload + 3 + tag_name_size); break;
                case NBT_SHORT: *((double*)va_dest) = nbt_short(payload + 3 + tag_name_size); break;
                case NBT_INT: *((double*)va_dest) = nbt_int(payload + 3 + tag_name_size); break;
                case NBT_LONG: *((double*)va_dest) = nbt_long(payload + 3 + tag_name_size); break;
                case NBT_FLOAT: *((double*)va_dest) = nbt_float(payload + 3 + tag_name_size); break;
                case NBT_DOUBLE: *((double*)va_dest) = nbt_double(payload + 3 + tag_name_size); break;
                }
            }

            else if (va_type == tag_type) {
                *((char**)va_dest) = payload + 3 + tag_name_size;
            }
        }

        va_name = va_arg(iter, char*);
        if (va_name != NULL) {
            va_name_size = va_arg(iter, size_t);
            va_type = va_arg(iter, int);
            va_dest = va_arg(iter, void*);
            goto next;
        }

        va_end(iter);

        payload = nbt_payload_step(payload + 3 + tag_name_size, tag_type, end);
    }

    return NULL;
}
