#pragma once

#include "anvil.h"

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

anvil_result anvil_set_errno(error_t err, const char *format, ...);
anvil_result anvil_set_result(anvil_result err, const char *format, ...);

#define anvil_assert(exp) (!(exp) ? ANVIL_OK :\
    anvil_set_result(ANVIL_INVALID_USAGE, "%s() (%s:%d) assertion failed %s", __func__, __FILE__, __LINE__, #exp)\
)
