#include <map.h>
#include <stdio.h>
#include <stdlib.h>

struct test {
	uint32_t as;
	void *ptr;
	char large[256];
	char _ext[];
};

int main() {
	map(void*, char) m;
	map_init(m, 8, 8, nullptr, nullptr, nullptr);

	map_set(m, "key");

	auto const t = map_get(m, "key");
	auto const t2 = *t;

	printf(map_get(m, "key"));

	map_free(m);
}
