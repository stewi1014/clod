#include "nbt.h"

#include <stdarg.h>
#include <string.h>

#define nbt_zero_size(type) (\
    (type) == NBT_BYTE ?       1 :\
    (type) == NBT_SHORT ?      2 :\
    (type) == NBT_INT ?        4 :\
    (type) == NBT_LONG ?       8 :\
    (type) == NBT_FLOAT ?      4 :\
    (type) == NBT_DOUBLE ?     8 :\
    (type) == NBT_BYTE_ARRAY ? 4 :\
    (type) == NBT_STRING ?     2 :\
    (type) == NBT_LIST ?       5 :\
    (type) == NBT_COMPOUND ?   1 :\
    (type) == NBT_INT_ARRAY ?  4 :\
    (type) == NBT_LONG_ARRAY ? 4 :\
    0)

#define nbt_zero_payload(type) (\
    (type) == NBT_BYTE ?        (char[1]){0} : \
    (type) == NBT_SHORT ?       (char[2]){0, 0} : \
    (type) == NBT_INT ?         (char[4]){0, 0, 0, 0} : \
    (type) == NBT_LONG ?        (char[8]){0, 0, 0, 0, 0, 0, 0, 0} : \
    (type) == NBT_FLOAT ?       (char[4]){0xFF, 0xFF, 0xFF, 0xFF} : \
    (type) == NBT_DOUBLE ?      (char[8]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} : \
    (type) == NBT_BYTE_ARRAY ?  (char[4]){0, 0, 0, 0} : \
    (type) == NBT_STRING ?      (char[2]){0, 0} : \
    (type) == NBT_LIST ?        (char[5]){0, 0, 0, 0, 0} : \
    (type) == NBT_COMPOUND ?    (char[1]){0} : \
    (type) == NBT_INT_ARRAY ?   (char[4]){0, 0, 0, 0} : \
    (type) == NBT_LONG_ARRAY ?  (char[4]){0, 0, 0, 0} : \
    (char[0]){})

char *nbt_type_string(const nbt_type type) {
    switch (type) {
    case NBT_NULL: return "NBT_END";
    case NBT_BYTE: return "NBT_BYTE";
    case NBT_SHORT: return "NBT_SHORT";
    case NBT_INT: return "NBT_INT";
    case NBT_LONG: return "NBT_LONG";
    case NBT_FLOAT: return "NBT_FLOAT";
    case NBT_DOUBLE: return "NBT_DOUBLE";
    case NBT_BYTE_ARRAY: return "NBT_BYTE_ARRAY";
    case NBT_STRING: return "NBT_STRING";
    case NBT_LIST: return "NBT_LIST";
    case NBT_COMPOUND: return "NBT_COMPOUND";
    case NBT_INT_ARRAY: return "NBT_INT_ARRAY";
    case NBT_LONG_ARRAY: return "NBT_LONG_ARRAY";
    default: return "NBT_INVALID";
    }
}

size_t nbt_payload_step(const char *const restrict payload, const char *const end, const nbt_type type) {
    if (!payload || !end || end < payload) return 0;
    switch(type) {
    case NBT_BYTE: return 1;
    case NBT_SHORT: return 2;
    case NBT_INT: return 4;
    case NBT_LONG: return 8;
    case NBT_FLOAT: return 4;
    case NBT_DOUBLE: return 8;
    case NBT_BYTE_ARRAY: {
        if (end - payload < 4) return 0;
        return 4 + (
            (uint8_t)payload[0] << (3 * 8) |
            (uint8_t)payload[1] << (2 * 8) |
            (uint8_t)payload[2] << (1 * 8) |
            (uint8_t)payload[3] << (0 * 8) );
    }
    case NBT_STRING: {
        if (end - payload < 2) return 0;
        return 2 + ((uint8_t)payload[0] << 8 | (uint8_t)payload[1]);
    }
    case NBT_LIST: {
        if (end - payload < 5) return 0;
        const nbt_type list_etype = payload[0];
        if (list_etype == NBT_NULL) return 5;

        const int32_t list_size =
            (uint8_t)payload[1] << (3 * 8) |
            (uint8_t)payload[2] << (2 * 8) |
            (uint8_t)payload[3] << (1 * 8) |
            (uint8_t)payload[4] << (0 * 8) ;
        if (list_size <= 0) return 5;

        if (nbt_type_is_dynamic(list_etype)) {
            size_t size = 5;
            for (int32_t i = 0; i < list_size; i++) {
                size += nbt_payload_step(payload + size, end, list_etype);
            }
            return size;
        }

        return 5 + list_size * nbt_payload_step(payload + 5, end, list_etype);
    }
    case NBT_COMPOUND: {
        const char *restrict cursor = payload;
        while (end - cursor >= 3 && nbt_type_is_valid(cursor[0])) {
            const uint16_t name_size =
                (uint8_t)cursor[1] << (1 * 8) |
                (uint8_t)cursor[2] << (0 * 8) ;

            cursor += 3 + name_size + nbt_payload_step(cursor + 3 + name_size, end, cursor[0]);
        }
        return cursor - payload + 1;
    }
    case NBT_INT_ARRAY: {
        const int32_t list_size =
            (uint8_t)payload[0] << (3 * 8) |
            (uint8_t)payload[1] << (2 * 8) |
            (uint8_t)payload[2] << (1 * 8) |
            (uint8_t)payload[3] << (0 * 8) ;

        return 4 + 4 * list_size;
    }
    case NBT_LONG_ARRAY: {
        const int32_t list_size =
            (uint8_t)payload[0] << (3 * 8) |
            (uint8_t)payload[1] << (2 * 8) |
            (uint8_t)payload[2] << (1 * 8) |
            (uint8_t)payload[3] << (0 * 8) ;

        return 4 + 8 * list_size;
    }
    default: return 0;
    }
}

size_t nbt_named(const char *const restrict payload, const char *const end,
                 const char *name, const size_t name_size, const nbt_type type, void *dest,
                 ...
) {
    if (!payload || !end) return 0;

    va_list va;
    va_start(va, dest);

    const char *restrict cursor = payload;
    while (end - cursor >= 3 && nbt_type_is_valid(cursor[0])) {
        const size_t tag_name_size = (uint8_t)cursor[1] << 8 | (uint8_t)cursor[2];
        if (end - cursor < 3 + tag_name_size) return 0;

        auto va_name = name;
        auto va_name_size = name_size;
        auto va_type = type;
        auto va_dest = dest;

        va_list iter;
        va_copy(iter, va);
    next_arg:

        void *extra[2];
        switch (va_type) {
        case NBT_BYTE_ARRAY: extra[0] = va_arg(iter, size_t*); break;
        case NBT_STRING:     extra[0] = va_arg(iter, size_t*); break;
        case NBT_LIST:       extra[0] = va_arg(iter, nbt_type*); extra[1] = va_arg(iter, size_t*); break;
        case NBT_INT_ARRAY:  extra[0] = va_arg(iter, size_t*); break;
        case NBT_LONG_ARRAY: extra[0] = va_arg(iter, size_t*); break;
        default: break;
        }

        if (
            tag_name_size == va_name_size &&
            0 == strncmp(cursor + 3, va_name, tag_name_size)
        ) {
            if (va_type == NBT_ANY_INTEGER && nbt_type_is_integer(cursor[0])) {
                switch (cursor[0]) {
                case NBT_BYTE:  *((int64_t*)va_dest) = (int64_t)nbt_read_byte( cursor + 3 + tag_name_size, end); break;
                case NBT_SHORT: *((int64_t*)va_dest) = (int64_t)nbt_read_short(cursor + 3 + tag_name_size, end); break;
                case NBT_INT:   *((int64_t*)va_dest) = (int64_t)nbt_read_int(  cursor + 3 + tag_name_size, end); break;
                case NBT_LONG:  *((int64_t*)va_dest) = (int64_t)nbt_read_long( cursor + 3 + tag_name_size, end); break;
                default: break;
                }
            } else if (va_type == NBT_ANY_NUMBER && nbt_type_is_number(cursor[0])) {
                switch (cursor[0]) {
                case NBT_BYTE:   *((double*)va_dest) = (double)nbt_read_byte(  cursor + 3 + tag_name_size, end); break;
                case NBT_SHORT:  *((double*)va_dest) = (double)nbt_read_short( cursor + 3 + tag_name_size, end); break;
                case NBT_INT:    *((double*)va_dest) = (double)nbt_read_int(   cursor + 3 + tag_name_size, end); break;
                case NBT_LONG:   *((double*)va_dest) = (double)nbt_read_long(  cursor + 3 + tag_name_size, end); break;
                case NBT_FLOAT:  *((double*)va_dest) = (double)nbt_read_float( cursor + 3 + tag_name_size, end); break;
                case NBT_DOUBLE: *((double*)va_dest) = (double)nbt_read_double(cursor + 3 + tag_name_size, end); break;
                default: break;
                }
            } else if (va_type == cursor[0]) {
                switch (va_type) {
                case NBT_BYTE:       *(int8_t*)va_dest      = nbt_read_byte(    cursor + 3 + tag_name_size, end); break;
                case NBT_SHORT:      *(int16_t*)va_dest     = nbt_read_short(   cursor + 3 + tag_name_size, end); break;
                case NBT_INT:        *(int32_t*)va_dest     = nbt_read_int(     cursor + 3 + tag_name_size, end); break;
                case NBT_LONG:       *(int64_t*)va_dest     = nbt_read_long(    cursor + 3 + tag_name_size, end); break;
                case NBT_FLOAT:      *(float*)va_dest       = nbt_read_byte(    cursor + 3 + tag_name_size, end); break;
                case NBT_DOUBLE:     *(double*)va_dest      = nbt_read_byte(    cursor + 3 + tag_name_size, end); break;
                case NBT_BYTE_ARRAY: *(const char**)va_dest = nbt_read_bytea(   cursor + 3 + tag_name_size, end, extra[0]); break;
                case NBT_STRING:     *(const char**)va_dest = nbt_read_string(  cursor + 3 + tag_name_size, end, extra[0]); break;
                case NBT_LIST:       *(const char**)va_dest = nbt_read_list(    cursor + 3 + tag_name_size, end, extra[0], extra[1]); break;
                case NBT_COMPOUND:   *(const char**)va_dest = cursor + 3 + tag_name_size; break;
                case NBT_INT_ARRAY:  *(const char**)va_dest = nbt_read_inta(    cursor + 3 + tag_name_size, end, extra[0]); break;
                case NBT_LONG_ARRAY: *(const char**)va_dest = nbt_read_longa(   cursor + 3 + tag_name_size, end, extra[0]); break;
                default: break;
                }
            }
        }

        va_name = va_arg(iter, char*);
        if (va_name != nullptr) {
            va_name_size = va_arg(iter, size_t);
            va_type = va_arg(iter, nbt_type);
            va_dest = va_arg(iter, void*);
            goto next_arg;
        }

        va_end(iter);
        const size_t payload_size = nbt_payload_step(cursor + 3 + tag_name_size, end, cursor[0]);
        if (payload_size == 0) return 0;
        cursor += 3 + tag_name_size + payload_size;
    }
    if (end - cursor < 1 || cursor[0] != NBT_NULL) return 0;
    return cursor - payload + 1;
}

ptrdiff_t nbt_write_tag(
    char *const restrict tag,
    const char *const end,
    const nbt_type type,
    const char *const name,
    const size_t name_size
) {
    if (!nbt_type_is_valid(type)) return 0;
    const size_t zero_size = nbt_zero_size(type);

    if (!tag) return (ptrdiff_t)(3 + name_size + zero_size);

    if (!end || end == tag) {
        tag[0] = type;
        tag[1] = (char)(name_size >> 8);
        tag[2] = (char)(name_size);
        memcpy(tag + 3, name, name_size);
        memcpy(tag + 3 + name_size, nbt_zero_payload(type), zero_size);
        return (ptrdiff_t)(3 + name_size + zero_size);
    }

    if (end - tag < 3 || !nbt_type_is_valid(tag[0])) return 0;
    const uint16_t old_name_size = (uint8_t)tag[1] << 8 | (uint8_t)tag[2];
    const size_t old_payload_size = nbt_payload_step(tag + 3 + old_name_size, end, tag[0]);

    if (end - tag < 3 + old_name_size) return 0;

    if (tag[0] == type) {
        memcpy(
            tag + 3 + name_size,
            tag + 3 + old_name_size,
            end - tag - 3 - old_name_size
        );
        tag[1] = (char)(name_size >> 8);
        tag[2] = (char)(name_size);
        memcpy(tag + 3, name, name_size);
        return (ptrdiff_t)(name_size - old_name_size);
    }

    memcpy(
        tag + 3 + name_size + zero_size,
        tag + 3 + old_name_size + old_payload_size,
        end - tag - 3 - old_name_size - old_payload_size
    );
    tag[0] = type;
    tag[1] = (char)(name_size >> 8);
    tag[2] = (char)(name_size);
    memcpy(tag + 3, name, name_size);
    return (ptrdiff_t)(name_size + zero_size - old_name_size - old_payload_size);
}
