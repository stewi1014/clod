#include "test.h"
#include "compression_test.c"
#include <stdlib.h>
#include <string.h>
#include <clod/compression.h>

int main() {
	compression_test(CLOD_LZMA);
}
