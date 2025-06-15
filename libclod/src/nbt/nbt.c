#include <alloca.h>
#include <assert.h>
#include <string.h>
#include <clod/nbt.h>

// caller has responsibility to ensure valid inputs.
// the return value is not checked for validity.
static size_t payload_size(const union clod_nbt_payload *restrict payload, const clod_nbt_type payload_type, const char *const end) {
	assert(payload);
	assert(end);
	assert(payload->ptr <= end);

	switch (payload_type) {
		default: return 0;
		case CLOD_NBT_INT8: return 1;
		case CLOD_NBT_INT16: return 2;
		case CLOD_NBT_INT32: return 4;
		case CLOD_NBT_INT64: return 8;
		case CLOD_NBT_FLOAT32: return 4;
		case CLOD_NBT_FLOAT64: return 8;
		case CLOD_NBT_INT8_ARRAY: {
			if (sizeof(payload->byte_array) > end - payload->ptr) return 0;
			return sizeof(payload->byte_array) + be(payload->byte_array.length);
		}
		case CLOD_NBT_INT32_ARRAY: {
			if (sizeof(payload->int32_array) > end - payload->ptr) return 0;
			return sizeof(payload->int32_array) + (size_t)be(payload->int32_array.length) * 4;
		}
		case CLOD_NBT_INT64_ARRAY: {
			if (sizeof(payload->int64_array) > end - payload->ptr) return 0;
			return sizeof(payload->int64_array) + (size_t)be(payload->int64_array.length) * 8;
		}
		case CLOD_NBT_STRING: {
			if (sizeof(payload->string) > end - payload->ptr) return 0;
			return sizeof(payload->string) + be(payload->string.length);
		}
		case CLOD_NBT_LIST: {
			if (sizeof(payload->list) > end - payload->ptr) return 0;
			size_t size = sizeof(payload->list);
			for (int32_t i = 0; i < be(payload->list.length) && size < end - payload->ptr; i++) {
				const size_t elem_size = payload_size((void*)(payload->ptr + size), payload->list.payload_type, end);
				if (elem_size == 0) return 0;
				size += elem_size;
			}
			return size;
		}
		case CLOD_NBT_COMPOUND: {
			size_t size = 0;
			while (size + sizeof(union clod_nbt_tag) < end - payload->ptr && payload->ptr[size] != CLOD_NBT_ZERO) {
				const union clod_nbt_tag *elem = (union clod_nbt_tag*)(payload->ptr + size);
				size += sizeof(union clod_nbt_tag) + be(elem->name_size);
				if (size > end - payload->ptr) return 0;
				size += payload_size((union clod_nbt_payload*)(payload->ptr + size), elem->type, end);
			}
			return size + 1;
		}
	}
}
size_t clod_nbt_payload_size(const union clod_nbt_payload *restrict payload, const clod_nbt_type payload_type, const char *const end) {
	assert(payload);
	assert(end);
	assert(payload->ptr <= end);
	const size_t size = payload_size(payload, payload_type, end);
	if (size > end - payload->ptr) return 0;
	return size;
}
size_t clod_nbt_tag_size(const union clod_nbt_tag *restrict tag, const char *const end) {
	assert(tag);
	assert(end);
	assert(tag->ptr <= end);
	if (sizeof(tag->type) > end - tag->ptr) return 0;
	if (tag->type == CLOD_NBT_ZERO) return sizeof(tag->type);
	if (sizeof(*tag) > end - tag->ptr) return 0;
	const size_t name_size = be(tag->name_size);
	if (sizeof(*tag) + name_size > end - tag->ptr) return 0;
	const size_t payload_size = clod_nbt_payload_size(clod_nbt_tag_payload(tag), tag->type, end);
	if (payload_size == 0) return 0;
	return sizeof(*tag) + name_size + payload_size;
}
void clod_nbt_get(const union clod_nbt_tag *tag, const char *end, const union clod_nbt_tag **children, const size_t children_count) {
	if (children_count == 0) return;
	const union clod_nbt_tag **children_in = alloca(sizeof(*children) * children_count);
	memcpy(children_in, children, sizeof(*children) * children_count);
	memset(children, 0, sizeof(*children) * children_count);

	if (
		tag->ptr > end ||
		end - tag->ptr < sizeof(*tag) ||
		tag->type != CLOD_NBT_COMPOUND ||
		end - tag->name < be(tag->name_size)
	) return;

	auto elem = clod_nbt_tag_payload(tag)->compound;
	size_t found = 0;
	while (found < children_count && end - elem->ptr >= sizeof(*elem) && elem->type != CLOD_NBT_ZERO) {
		const size_t elem_name_size = be(elem->name_size);
		if (end - elem->name < elem_name_size) return;

		for (size_t i = 0; i < children_count; i++) {
			if (
				!children[i] &&
				children_in[i]->type == elem->type &&
				be(children_in[i]->name_size) == elem_name_size &&
				memcmp(children_in[i]->name, elem->name, elem_name_size) == 0
			) {
				children[i] = elem;
				found++;
			}
		}

		const size_t elem_size = clod_nbt_tag_size(elem, end);
		if (end - elem->ptr < elem_size || elem_size == 0) return;
		elem = (union clod_nbt_tag*)(elem->ptr + elem_size);
	}
}
