#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "nbt.h"

inline const char *nbt_memshift(char *ptr, const char *end, size_t old_size, size_t new_size) {
    __nbt_assert(ptr != nullptr);
    __nbt_assert(end != nullptr);
    __nbt_assert(end >= ptr);

    // if old_size is larger than the apparent size of the entire buffer,
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

char *nbt_step(char *tag, const char *end) {
    __nbt_assert(tag != nullptr);
    if (!__nbt_has_data(tag, end, 1)) return nullptr;
    const char type = tag[0];

    if (!nbt_type_is_valid(type)) return nullptr;
    if (type == NBT_END) return tag + 1;

    if (!__nbt_has_data(tag, end, 3)) return nullptr;
    const uint16_t name_size = nbt_name_size(tag, end);

    if (!__nbt_has_data(tag, end, 3 + (size_t)name_size)) return nullptr;
    return nbt_payload_step(tag + 3 + (size_t)name_size, type, end);
}

char *nbt_payload_step(char *payload, const char type, const char *end) {
    __nbt_assert(payload != nullptr && nbt_type_has_payload(type));
    if (!__nbt_has_data(payload, end, 1)) return nullptr;

    switch (type) {
    case NBT_BYTE: return __nbt_has_data(payload, end, 1) ? payload + 1 : nullptr;
    case NBT_SHORT: return __nbt_has_data(payload, end, 2) ? payload + 2 : nullptr;
    case NBT_INT: return __nbt_has_data(payload, end, 4) ? payload + 4 : nullptr;
    case NBT_LONG: return __nbt_has_data(payload, end, 8) ? payload + 8 : nullptr;
    case NBT_FLOAT: return __nbt_has_data(payload, end, 4) ? payload + 4 : nullptr;
    case NBT_DOUBLE: return __nbt_has_data(payload, end, 8) ? payload + 8 : nullptr;
    case NBT_BYTE_ARRAY: {
        if (!__nbt_has_data(payload, end, 4)) return nullptr;
        const int32_t size = nbt_byte_array_size(payload);

        if (!__nbt_has_data(payload, end, 4 + (ptrdiff_t)size)) return nullptr;
        return payload + 4 + (ptrdiff_t)size;
    }

    case NBT_STRING: {
        if (!__nbt_has_data(payload, end, 2)) return nullptr;
        const uint16_t size = nbt_string_size(payload);

        if (!__nbt_has_data(payload, end, 2 + (size_t)size)) return nullptr;
        return payload + 2 + (size_t)size;
    }

    case NBT_LIST: {
        if (!__nbt_has_data(payload, end, 5)) return nullptr;
        const auto etype = nbt_list_etype(payload);
        const auto size = nbt_list_size(payload);
        payload = nbt_list_payload(payload);
        switch (etype) {
        case NBT_END: return __nbt_has_data(payload, end, 0) ? payload : nullptr;
        case NBT_BYTE: return __nbt_has_data(payload, end, (ptrdiff_t)size) ? (payload + (ptrdiff_t)size) : nullptr;
        case NBT_SHORT: return __nbt_has_data(payload, end, (ptrdiff_t)size * 2) ? (payload + (ptrdiff_t)size * 2) : nullptr;
        case NBT_INT: return __nbt_has_data(payload, end, (ptrdiff_t)size * 4) ? (payload + (ptrdiff_t)size * 4) : nullptr;
        case NBT_LONG: return __nbt_has_data(payload, end, (ptrdiff_t)size * 8) ? (payload + (ptrdiff_t)size * 8) : nullptr;
        case NBT_FLOAT: return __nbt_has_data(payload, end, (ptrdiff_t)size * 4) ? (payload + (ptrdiff_t)size * 4) : nullptr;
        case NBT_DOUBLE: return __nbt_has_data(payload, end, (ptrdiff_t)size * 8) ? (payload + (ptrdiff_t)size * 8) : nullptr;
        case NBT_BYTE_ARRAY:
        case NBT_STRING:
        case NBT_LIST:
        case NBT_COMPOUND:
        case NBT_INT_ARRAY:
        case NBT_LONG_ARRAY: {
            for (int32_t i=0; i<size && payload != nullptr; i++)
                payload = nbt_payload_step(payload, etype, end);
            return payload;
        }
        default: return nullptr;
        }
    }

    case NBT_COMPOUND: {
        while (payload != nullptr && payload[0] != NBT_END)
            payload = nbt_step(payload, end); 
        return payload != nullptr && __nbt_has_data(payload, end, 1) ? payload+1 : nullptr;
    }

    case NBT_INT_ARRAY: {
        if (!__nbt_has_data(payload, end, 4)) return nullptr;
        const auto size = nbt_int_array_size(payload);
        if (!__nbt_has_data(payload, end, 4 + 4 * (ptrdiff_t)size)) return nullptr;
        return payload + 4 + 4 * (ptrdiff_t)size;
    }

    case NBT_LONG_ARRAY: {
        if (!__nbt_has_data(payload, end, 4)) return nullptr;
        const auto size = nbt_long_array_size(payload);
        if (!__nbt_has_data(payload, end, 4 + 8 * (ptrdiff_t)size)) return nullptr;
        return payload + 4 + 8 * (ptrdiff_t)size;
    }

    default: return nullptr;
    }
}


//=================//
// Syntactic Sugar //
//=================//

char *nbt_named(char *payload, const char *end,
    const char *name, const size_t name_size, const int type, void *dest,
    ...
) {
    if (payload == nullptr) {
        return nullptr;
    }

    va_list va = {nullptr};
    va_start(va, dest);

    while (payload != nullptr) {
        if (!__nbt_has_data(payload, end, 1)) {
            return nullptr;
        }

        const char tag_type = payload[0];
        if (tag_type == NBT_END) {
            return payload + 1;
        }

        if (!nbt_type_is_valid(tag_type) || !__nbt_has_data(payload, end, 3)) {
            return nullptr;
        }

        const uint16_t tag_name_size = nbt_name_size(payload, end);

        if (!__nbt_has_data(payload, end, 3 + tag_name_size)) {
            return nullptr;
        }

        const char *va_name = name;
        size_t va_name_size = name_size;
        auto va_type = type;
        void *va_dest = dest;
        va_list iter;
        va_copy(iter, va);

    next:
        if (tag_name_size == va_name_size && 0 == strncmp(nbt_name(payload, end), va_name, va_name_size)) {
            if (va_type == NBT_ANY_INTEGER && nbt_type_is_integer(tag_type)) {
                switch (tag_type) {
                case NBT_BYTE: *((int64_t*)va_dest) = (int64_t)nbt_byte(payload + 3 + tag_name_size); break;
                case NBT_SHORT: *((int64_t*)va_dest) = (int64_t)nbt_short(payload + 3 + tag_name_size); break;
                case NBT_INT: *((int64_t*)va_dest) = (int64_t)nbt_int(payload + 3 + tag_name_size); break;
                case NBT_LONG: *((int64_t*)va_dest) = nbt_long(payload + 3 + tag_name_size); break;
                default: return nullptr;
                }
            }

            else if (va_type == NBT_ANY_NUMBER && nbt_type_is_number(tag_type)) {
                switch (tag_type) {
                case NBT_BYTE: *((double*)va_dest) = (double)nbt_byte(payload + 3 + tag_name_size); break;
                case NBT_SHORT: *((double*)va_dest) = (double)nbt_short(payload + 3 + tag_name_size); break;
                case NBT_INT: *((double*)va_dest) = (double)nbt_int(payload + 3 + tag_name_size); break;
                case NBT_LONG: *((double*)va_dest) = (double)nbt_long(payload + 3 + tag_name_size); break;
                case NBT_FLOAT: *((double*)va_dest) = (double)nbt_float(payload + 3 + tag_name_size); break;
                case NBT_DOUBLE: *((double*)va_dest) = nbt_double(payload + 3 + tag_name_size); break;
                default: return nullptr;
                }
            }

            else if (va_type == tag_type) {
                *((char**)va_dest) = payload + 3 + tag_name_size;
            }
        }

        va_name = va_arg(iter, char*);
        if (va_name != nullptr) {
            va_name_size = va_arg(iter, size_t);
            va_type = va_arg(iter, int);
            va_dest = va_arg(iter, void*);
            goto next;
        }

        va_end(iter);

        payload = nbt_payload_step(payload + 3 + tag_name_size, tag_type, end);
    }

    return nullptr;
}
