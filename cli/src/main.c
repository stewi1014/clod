/**
 * @defgroup clod clod
 * @brief CLI tool
 *
 */

#include <math.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>


#define REGION_EXTENT(coord_count) (size_t)(\
    (coord_count) == 0 ? 1      :\
    (coord_count) == 1 ? 1024   :\
    (coord_count) == 2 ? 32     :\
    (coord_count) == 3 ? 11     :\
    (coord_count) == 4 ? 6      :\
    (coord_count) == 5 ? 4      :\
    (coord_count) == 6 ? 3      :\
    (coord_count) == 7 ? 3      :\
    (coord_count) == 8 ? 2      :\
    (coord_count) == 9 ? 2      :\
    (coord_count) == 10 ? 2     :\
    0\
)
#define REGION_CHUNK_COUNT(coord_count) ((size_t)pow(REGION_EXTENT(coord_count), (coord_count)))

int main(int argc, char **argv) {
    for (int64_t i = 0; i <= 10; i++) {
        printf("%"PRId64": %"PRId64" %"PRId64"\n", i, REGION_EXTENT(i), REGION_CHUNK_COUNT(i));
    }
};
