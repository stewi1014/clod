#include "nbt.h"

char *nbt_payload_step(char type, char *payload) {
    if (payload == __nbt_null) return __nbt_null;
    switch (type) {
    case NBT_BYTE: return payload + 1;
    case NBT_SHORT: return payload + 2;
    case NBT_INT: return payload + 4;
    case NBT_LONG: return payload + 8;
    case NBT_FLOAT: return payload + 4;
    case NBT_DOUBLE: return payload + 8;
    case NBT_BYTE_ARRAY: return payload + 4 + nbt_byte_array_size(payload);
    case NBT_STRING: return payload + 2 + nbt_string_size(payload);
    case NBT_LIST: {
        char etype = nbt_list_etype(payload);
        int size = nbt_list_size(payload);
        payload = nbt_list(payload);
        switch (etype) {
        case NBT_END: return payload;
        case NBT_BYTE: return payload + size;
        case NBT_SHORT: return payload + (size * 2);
        case NBT_INT: return payload + (size * 4);
        case NBT_LONG: return payload + (size * 8);
        case NBT_FLOAT: return payload + (size * 4);
        case NBT_DOUBLE: return payload + (size * 8);
        case NBT_BYTE_ARRAY: [[fallthrough]];
        case NBT_STRING: [[fallthrough]];
        case NBT_LIST: [[fallthrough]];
        case NBT_COMPOUND: [[fallthrough]];
        case NBT_INT_ARRAY: [[fallthrough]];
        case NBT_LONG_ARRAY: for (int i=0; i<size && payload != __nbt_null; i++) payload = nbt_payload_step(etype, payload); return payload;
        default: return __nbt_null;
        }
    }
    case NBT_COMPOUND: while (payload != __nbt_null && payload[0] != NBT_END) payload = nbt_step(payload); return payload == __nbt_null ? __nbt_null : payload+1;
    case NBT_INT_ARRAY: return payload + 4 + 4 * nbt_int_array_size(payload);
    case NBT_LONG_ARRAY: return payload + 4 + 8 * nbt_long_array_size(payload);
    default: return __nbt_null;
    }
}
