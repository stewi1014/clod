#ifndef CLOD_REGION_ERROR_H
#define CLOD_REGION_ERROR_H

#include <clod/region.h>

__attribute__((cold)) __attribute__((format(printf, 4, 5)))
enum clod_region_result
print_error(enum clod_region_result result, const char *source_name, int source_line, const char *msg, ...);
#define region_error(result, msg, ...) print_error(result, __FILE__, __LINE__, msg, ##__VA_ARGS__)

#endif
