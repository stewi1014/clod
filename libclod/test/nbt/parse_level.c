#include <stdlib.h>

#include "test.h"
#include <clod/nbt.h>

const char level_data[] = {
#embed "level.nbt"
};

int main() {
	const size_t res = clod_nbt_tag_size((void*)level_data, level_data + sizeof(level_data));
	check(res == sizeof(level_data));
}
