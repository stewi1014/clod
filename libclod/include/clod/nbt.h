/**
 * @file nbt.h
 * @brief Methods for dealing with Named Binary Tag (NBT) data.
 *
 * The NBT format is a serialised depth-first tree with no indexing.
 * The primary limitation of interacting with such a structure is simply figuring out where the nodes actually are.
 * To find a given NBT, every single tag before it must be recursively parsed to find the wanted NBT's offset.
 * As such, traversing the tree becomes a core function of any NBT operation and the primary focus for optimisation.
 * This traversal method is nbt_payload_size.
 *
 * Implementing complex data analysis to discern the likelihood that a given set of NBT data
 * has been modified from its original state is an insane alternative to using a checksum and out of scope for this library.
 * Please, for the love of good code, rely on dedicated verification methods instead of incidental parsing errors.
 */
#ifndef CLOD_NBT_H
#define CLOD_NBT_H

#include <clod/lib.h>
#include <clod/big_endian.h>
#include <stddef.h>

#define CLOD_NBT_ZERO        0
#define CLOD_NBT_INT8        1
#define CLOD_NBT_INT16       2
#define CLOD_NBT_INT32       3
#define CLOD_NBT_INT64       4
#define CLOD_NBT_FLOAT32     5
#define CLOD_NBT_FLOAT64     6
#define CLOD_NBT_INT8_ARRAY  7
#define CLOD_NBT_INT32_ARRAY 11
#define CLOD_NBT_INT64_ARRAY 12
#define CLOD_NBT_STRING      8
#define CLOD_NBT_LIST        9
#define CLOD_NBT_COMPOUND    10

typedef signed char clod_nbt_type;

/** Named Binary Tag. */
union clod_nbt_tag {
	// Implementation helper to get the address of the tag in units of 1.
	char ptr[];

	struct {
		// Type of the NBT.
		clod_nbt_type type;

		// Size of the name of the tag.
		beu16 name_size;

		// Name of the tag. Not zero delimited.
		char name[/* name_size */];

		/* payload follows */
	};
};

// Get the payload for a tag.
// Does not perform bound checks.
#define clod_nbt_tag_payload(tag) ((union clod_nbt_payload *)((tag)->ptr + sizeof(union clod_nbt_tag) + be((tag)->name_size)))

/** Union of all NBT payload types. */
union clod_nbt_payload {
	// Implementation helper to get the address of the payload in units of 1.
	char ptr[];

	// NBT Byte payload
	bei8 int8;

	// NBT Short payload
	bei16 int16;

	// NBT Int payload
	bei32 int32;

	// NBT Long payload
	bei64 int64;

	// NBT Float payload
	bef32 float32;

	// NBT Double payload
	bef64 float64;

	// NBT Byte Array payload
	struct {
		bei32 length;
		bei8 data[/* length */];
	} byte_array;

	// NBT String payload
	struct {
		beu16 length;
		char data[/* length */];
	} string;

	// NBT List payload
	struct {
		clod_nbt_type payload_type;
		bei32 length;
		char data[/* length */];
	} list;

	// NBT Compound payload
	// Delimited by a zero-type NBT tag of size 1.
	union clod_nbt_tag compound[];

	// Int Array
	struct {
		bei32 length;
		bei32 data[/* length */];
	} int32_array;

	// Long Array
	struct {
		bei32 length;
		bei64 data[/* length */];
	} int64_array;
};

static_assert(CHAR_BIT == 8);
static_assert(alignof(union clod_nbt_tag) == 1);
static_assert(alignof(union clod_nbt_payload) == 1);

/**
 * Get the size of an NBT payload.
 * This is the primary NBT traversing function; everything else is built on top of this.
 *
 * @param[in] payload The payload to get the size of.
 * @param[in] payload_type The type of the payload.
 * @param[in] end The end of the NBT data.
 * The method will never read past end.
 * @return The size of the payload, or 0 on invalid arguments.
 * Undefined if NBT data is malformed.
 */
CLOD_API CLOD_NONNULL(1, 3)
size_t
clod_nbt_payload_size(const union clod_nbt_payload *payload, clod_nbt_type payload_type, const char *end);

/**
 * Get the size of an NBT tag.
 * It's mostly a wrapper around nbt_payload_size.
 *
 * @param[in] tag The tag to get the size of.
 * @param[in] end The end of the NBT data.
 * The method will never read past end.
 * @return Size of the tag, or 0 on invalid arguments.
 * Undefined if NBT data is malformed.
 */
CLOD_API CLOD_NONNULL(1, 2)
size_t
clod_nbt_tag_size(const union clod_nbt_tag *tag, const char *end);

/**
 * Get children from a compound tag.
 *
 * @param[in] tag The compound tag to search through.
 * @param[in] end The end of the NBT data.
 * @param[in,out] children The child tags to search for.
 * @param[in] children_count Length of children.
 */
CLOD_API CLOD_NONNULL(1, 2, 3)
void
clod_nbt_get(const union clod_nbt_tag *tag, const char *end, const union clod_nbt_tag **children, size_t children_count);

#endif
