#include "clod_config.h"
#include "error.h"

#include <stdio.h>
#include <stdarg.h>

const char *result_string(const enum clod_region_result result) {
	switch (result) {
		case CLOD_REGION_OK: return "CLOD_REGION_OK";
		case CLOD_REGION_INVALID_USAGE: return "CLOD_REGION_INVALID_USAGE";
		case CLOD_REGION_MALFORMED: return "CLOD_REGION_MALFORMED";
		case CLOD_REGION_NOT_FOUND: return "CLOD_REGION_NOT_FOUND";
	}
	return "CLOD_REGION_RESULT_UNKNOWN";
}

enum clod_region_result print_error(const enum clod_region_result result, const char *source_name, const int source_line, const char *msg, ...) {
	char buff[1024] = {0};
	va_list va;
	va_start(va, msg);
	vsnprintf(buff, sizeof(buff), msg, va);
	va_end(va);
	fprintf(stderr, "%s ("CLOD_COMMIT_HASH":%s:%d): %s\n", result_string(result), source_name, source_line, buff);
	return result;
}
