#include <stdio.h>
#include <string.h>

#include "test.h"

#define RUN_TEST(name)\
if (argc <= 1) {\
    int r = name();\
    if (!r) printf("test: " #name " ✅\n");\
    else (printf("test: " #name " ❌\n"), failed = 1);\
} else for (int i = 1; i < argc; i++) if (!strcmp(argv[i], #name)) {\
    int r = name();\
    if (!r) printf("test: " #name " ✅\n");\
    else (printf("test: " #name " ❌\n"), failed = 1);\
}

int main(int argc, char **argv) {
    int failed = 0;

    RUN_TEST(open_region);
    RUN_TEST(open_region_nokeep)
    RUN_TEST(parse_raw_nbt);
    //RUN_TEST(parse_gzip_nbt);
    //RUN_TEST(parse_zlib_nbt);

    return failed;
}
