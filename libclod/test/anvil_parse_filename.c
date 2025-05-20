#include <string.h>

#include "anvil.h"
#include "test.h"

int test(
    const char *filename,
    const char *prefix,
    const char *extension,
    const int64_t x,
    const int64_t z
) {
    char test_buffer[255];

    const size_t filename_size = anvil_create_filename(
        test_buffer, sizeof(test_buffer),
        prefix, extension, 2, x, z
    );

    TEST_ASSERT(filename_size == strlen(filename) + 1)
    TEST_ASSERT(filename_size == strlen(test_buffer) + 1)
    TEST_ASSERT(0 == strcmp(filename, test_buffer))

    size_t got_prefix_size;
    const char *got_extension;
    int64_t got_x, got_z;
    const int64_t got_coords = anvil_parse_filename(
        test_buffer, sizeof(test_buffer),
        &got_prefix_size, &got_extension, 2, &got_x, &got_z
    );

    TEST_ASSERT(got_prefix_size == strlen(prefix))
    TEST_ASSERT(0 == strcmp(got_extension, extension))
    TEST_ASSERT(got_coords == 2);
    TEST_ASSERT(got_x == x);
    TEST_ASSERT(got_z == z);

    return 0;
}

TEST_MAIN(test) {
    TEST_CASE("r.-2.3.mca", "r", "mca", -2, 3);
    TEST_CASE("r.2.3.mca", "r", "mca", 2, 3);
    TEST_CASE("r.-2.-3.mca", "r", "mca", -2, -3);
    TEST_CASE(".0.0.", "", "", 0, 0);

    TEST_CASE("lod_data.-2.3.mca.gz", "lod_data", "mca.gz", -2, 3);
    TEST_CASE("lod_data.2.3.mca.gz", "lod_data", "mca.gz", 2, 3);
    TEST_CASE("lod_data.-2.-3.mca.gz", "lod_data", "mca.gz", -2, -3);
    TEST_CASE("lod_data.0.0.mca.gz", "lod_data", "mca.gz", 0, 0);
}
