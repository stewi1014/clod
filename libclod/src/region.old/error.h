#ifndef ERROR_H
#define ERROR_H

#include <clod/region.h>
#include <stdio.h>

struct error_details {
	enum region_result result;
	int source_line;
	const char *source_name;
	char msg[1008];
};
static_assert(sizeof(struct error_details) == 1024);

#define set_error(err, result, format, ...) set_error_impl((err), (result), __FILE__, __LINE__, format, ##__VA_ARGS__);
__attribute__((cold)) __attribute__((format(printf, 5, 6)))
void set_error_impl(struct error_details *err, enum region_result result,
	const char *source_name, int source_line, const char *format, ...);

#define log_error(details, opts) ((opts).logger((details)->result, (opts).logger_user, (details)->source_name, (details)->source_line, (details)->msg))
void default_logger(enum region_result result,
	void *user,
	const char *source_name,
	int source_line,
	const char *msg);

#endif
