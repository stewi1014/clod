#include "error.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>

const char *region_commit_hash = COMMIT_HASH;
static const char *result_strings[] = {
	[REGION_OK] = "Ok",
	[REGION_INVALID_USAGE] = "Invalid usage",
	[REGION_MALFORMED] = "Malformed data",
};
#define result_string(result) ((result) < (sizeof(result_strings) / sizeof(result_strings[0])) ? result_strings[result] : "Unknown result")

__attribute__((cold)) __attribute__((format(printf, 5, 6)))
void set_error_impl(struct error_details *err, enum region_result result,
	const char *source_name, int source_line, const char *format, ...) {
	err->result = result;
	err->source_name = source_name;
	err->source_line = source_line;

	va_list va;
	va_start(va, format);
	vsnprintf(err->msg, sizeof(err->msg), format, va);
	va_end(va);
}

void default_logger(enum region_result result,
	void *,
	const char *source_name,
	int source_line,
	const char *msg) {
	fprintf(stderr, "%s (%s:%s:%d): %s.",
		result_string(result), region_commit_hash, source_name, source_line, msg);
}
