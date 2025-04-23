#include <stdlib.h>
#include <mc.h>

char *nbt_parse_payload(char type, union MC_nbt_payload *payload, char *data, int depth);
void nbt_free_payload(char type, union MC_nbt_payload *payload);

char *MC_nbt_parse(struct MC_nbt *nbt, char *data, int depth) {
    if (data == NULL) return NULL;

    char type = data[0];
    if (nbt != NULL) nbt->type = type;
    if (type == MC_NBT_END) return data+1;

    unsigned short name_size = 
        ((unsigned short)(unsigned char)data[1]<<8) + 
        ((unsigned short)(unsigned char)data[2]);

    char *name = &data[3];
    if (nbt != NULL) {
        nbt->name_size = name_size;
        nbt->name = name;
    }

    if (depth < 0) {
        char *r = nbt_parse_payload(type, NULL, data+3+name_size, depth-1);

        if (nbt != NULL) {
            nbt->type = MC_NBT_UNPARSED;
            nbt->payload.unparsed.data = data;
            nbt->payload.unparsed.size_v = r - data;
        }

        return r;
    } else {
        return nbt_parse_payload(type, nbt != NULL ? &nbt->payload : NULL, data+3+name_size, depth-1);
    }
}

char *nbt_parse_payload(char type, union MC_nbt_payload *payload, char *data, int depth) {
    if (data == NULL) return NULL;

    switch(type) {
    case MC_NBT_UNPARSED:
        return data;

    case MC_NBT_END:
        return data;

    case MC_NBT_BYTE:
        if (payload != NULL) payload->byte_v = data[0];
        return data+1;

    case MC_NBT_SHORT:
        if (payload != NULL) payload->short_v = 
            ((short)(unsigned char)data[0]<<8) +
            ((short)(unsigned char)data[1]);
        return data+2;

    case MC_NBT_INT:
    case MC_NBT_FLOAT:
        if (payload != NULL) payload->int_v = 
            ((int)(unsigned char)data[0]<<24) + 
            ((int)(unsigned char)data[1]<<16) + 
            ((int)(unsigned char)data[2]<<8) + 
            ((int)(unsigned char)data[3]);
        return data+4;

    case MC_NBT_LONG:
    case MC_NBT_DOUBLE:
        if (payload != NULL) payload->long_v = 
            ((long)(unsigned char)data[0]<<56) + 
            ((long)(unsigned char)data[1]<<48) + 
            ((long)(unsigned char)data[2]<<40) + 
            ((long)(unsigned char)data[3]<<32) + 
            ((long)(unsigned char)data[4]<<24) + 
            ((long)(unsigned char)data[5]<<16) + 
            ((long)(unsigned char)data[6]<<8) + 
            ((long)(unsigned char)data[7]);
        return data+8;

    case MC_NBT_BYTE_ARRAY: {
        int size = 
            ((int)(unsigned char)data[0]<<24) + 
            ((int)(unsigned char)data[1]<<16) + 
            ((int)(unsigned char)data[2]<<8) + 
            ((int)(unsigned char)data[3]);

        if (payload != NULL) {
            payload->byte_array.array_v = &data[4];
            payload->byte_array.size_v = size;
        }

        return data+4+size;
    }

    case MC_NBT_STRING: {
        unsigned short size = 
            ((unsigned short)(unsigned char)data[0]<<8) + 
            ((unsigned short)(unsigned char)data[1]);

        if (payload != NULL) {
            payload->string.array_v = &data[2];
            payload->string.size_v = size;
        }

        return data+2+size;
    }

    case MC_NBT_LIST: {
        char etype = data[0];
        int size = 
            ((int)(unsigned char)data[1]<<24) + 
            ((int)(unsigned char)data[2]<<16) + 
            ((int)(unsigned char)data[3]<<8) + 
            ((int)(unsigned char)data[4]);
        data+=5;

        union MC_nbt_payload *array = NULL;
        if (payload != NULL) array = malloc(size * sizeof(union MC_nbt_payload));

        if (array != NULL)
            for (int i = 0; i < size; i++) data = nbt_parse_payload(etype, &array[i], data, depth-1);
        else
            for (int i = 0; i < size; i++) data = nbt_parse_payload(etype, NULL, data, depth-1);
        
        if (payload != NULL) {
            payload->list.array_v = array;
            payload->list.size_v = size;
            payload->list.type_v = etype;
        }

        return data;
    }

    case MC_NBT_COMPOUND: {
        int cap = 64, i = 0;
        struct MC_nbt *array = NULL;
        if (payload != NULL) array = malloc(cap * sizeof(struct MC_nbt));

        while (data != NULL && data[0] != MC_NBT_END) {
            if (i >= cap && array != NULL) {
                struct MC_nbt *new = realloc(array, (cap*=4) * sizeof(struct MC_nbt));
                if (new == NULL) {
                    free(array);
                    array = NULL;
                } else {
                    array = new;
                }
            }

            data = MC_nbt_parse(array != NULL ? &array[i++] : NULL, data, depth-1);
        }

        if (array != NULL) array[i].type = MC_NBT_END;
        if (payload != NULL) {
            payload->compound.array_v = array;
            payload->compound.size_v = i;
        }
        return data != NULL ? data+1 : NULL;
    }


    case MC_NBT_INT_ARRAY: {
        int size = 
            ((int)(unsigned char)data[0]<<24) + 
            ((int)(unsigned char)data[1]<<16) + 
            ((int)(unsigned char)data[2]<<8) + 
            ((int)(unsigned char)data[3]);
        data+=4;

        int *array = NULL;
        if (payload != NULL) array = malloc(size * sizeof(int));
        if (array != NULL) for (int i = 0; i < size; i++)
            array[i] = 
                ((int)(unsigned char)data[(i*4)+0]<<24) + 
                ((int)(unsigned char)data[(i*4)+1]<<16) + 
                ((int)(unsigned char)data[(i*4)+2]<<8) + 
                ((int)(unsigned char)data[(i*4)+3]);

        if (payload != NULL) {
            payload->int_array.array_v = array;
            payload->int_array.size_v = size;
        }

        return data+(size*4);
    }

    case MC_NBT_LONG_ARRAY: {
        int size = 
            ((int)(unsigned char)data[0]<<24) + 
            ((int)(unsigned char)data[1]<<16) + 
            ((int)(unsigned char)data[2]<<8) + 
            ((int)(unsigned char)data[3]);
        data+=4;
        
        long *array = NULL;
        if (payload != NULL) array = malloc(size * sizeof(long));
        if (array != NULL) for (int i = 0; i < size; i++)
            array[i] = 
                ((long)(unsigned char)data[(i*8)+0]<<56) + 
                ((long)(unsigned char)data[(i*8)+1]<<48) + 
                ((long)(unsigned char)data[(i*8)+2]<<40) + 
                ((long)(unsigned char)data[(i*8)+3]<<32) + 
                ((long)(unsigned char)data[(i*8)+4]<<24) + 
                ((long)(unsigned char)data[(i*8)+5]<<16) + 
                ((long)(unsigned char)data[(i*8)+6]<<8) + 
                ((long)(unsigned char)data[(i*8)+7]);

        if (payload != NULL) {
            payload->long_array.array_v = array;
            payload->long_array.size_v = size;
        }

        return data+(size*8);

        return NULL;
    }}

    return NULL;
}

void MC_nbt_free(struct MC_nbt *nbt) {
    if (nbt != NULL) nbt_free_payload(nbt->type, &nbt->payload);
}

void nbt_free_payload(char type, union MC_nbt_payload *payload) {
    switch (type) {
    case MC_NBT_UNPARSED: return;
    case MC_NBT_END: return;
    case MC_NBT_BYTE: return;
    case MC_NBT_SHORT: return;
    case MC_NBT_INT: return;
    case MC_NBT_LONG: return;
    case MC_NBT_FLOAT: return;
    case MC_NBT_DOUBLE: return;
    case MC_NBT_BYTE_ARRAY: return;
    case MC_NBT_STRING: return;
    case MC_NBT_LIST:
        if (payload->list.array_v != NULL) {
            for (int i=0; i<payload->list.size_v; i++) nbt_free_payload(payload->list.type_v, &payload->list.array_v[i]);
            free(payload->list.array_v);
            payload->list.array_v = NULL;
        }
        return;
    case MC_NBT_COMPOUND:
        if (payload->compound.array_v != NULL) {
            for (int i=0; i<payload->compound.size_v; i++) MC_nbt_free(&payload->compound.array_v[i]);
            free(payload->compound.array_v);
            payload->compound.array_v = NULL;
        }
        return;
    case MC_NBT_INT_ARRAY:
        if (payload->int_array.array_v != NULL) {
            free(payload->int_array.array_v);
            payload->int_array.array_v = NULL;
        }
        return;
    case MC_NBT_LONG_ARRAY:
        if (payload->long_array.array_v != NULL) {
            free(payload->long_array.array_v);
            payload->long_array.array_v = NULL;
        }
        return;
    }
}
