#pragma once
#include "config.h"
#include <anvil.h>

#include <errno.h>

#ifdef NDEBUG
    #define anvil_breakpoint()
#else
    #ifdef _MSC_VER
        #define anvil_breakpoint() __debugbreak()
    #else
        #include <signal.h>
        #define anvil_breakpoint() raise(SIGTRAP)
    #endif
#endif

#define MAX_ERROR_MSG 1024

anvil_result get_result(error_t err);
const char *strresult(anvil_result res);

anvil_result error(anvil_result res, const char *err_str, const char *format, ...);
#define error_errno(format, ...) error(get_result(errno), strerror(errno), format, ##__VA_ARGS__)
#define error_result(res, format, ...) error(res, strresult(res), format, ##__VA_ARGS__)
#define error_assert(expr) ((expr) ? ANVIL_OK :(\
    error(ANVIL_INVALID_USAGE, "Assertion failed", "%s()[%s:%d] %s", __func__, __FILE__, __LINE__, #expr),\
    ANVIL_INVALID_USAGE\
))

void pop_error(anvil_result *res, char *msg);
void push_error(anvil_result res, char *msg);
