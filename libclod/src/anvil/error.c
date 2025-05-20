#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "error.h"

#define MAX_ERROR_MSG 1024

_Thread_local anvil_result __anvil_errno_value = ANVIL_OK;
_Thread_local char __anvil_error_message_value[MAX_ERROR_MSG] = {0};

anvil_result anvil_error() {
    return __anvil_errno_value;
}

const char *anvil_error_message() {
    __anvil_error_message_value[MAX_ERROR_MSG - 1] = '\0'; // just to be extra safe.
    return __anvil_error_message_value;
}

void anvil_clear_error() {
    __anvil_errno_value = ANVIL_OK;
    __anvil_error_message_value[0] = '\0';
}

FILE *anvil_log = stderr;

const char *anvil_strerror(const anvil_result res) {
    switch (res) {
    case ANVIL_OK: return "Ok";
    case ANVIL_MALFORMED: return "Malformed";
    case ANVIL_ERROR_IO: return "IO Error";
    case ANVIL_INVALID_USAGE: return "Invalid usage";
    case ANVIL_ALLOC_FAILED: return "Alloc failed";
    case ANVIL_LOCKED: return "Locked";
    case ANVIL_NOT_EXIST: return "Does not exist";
    case ANVIL_NO_SPACE: return "No space on device";
    default: return "Unknown";
    }
}

anvil_result anvil_errno_mapping(const error_t err) {
    switch (err) {
    case EPERM: return ANVIL_MALFORMED;

    case EBADF:        return ANVIL_INVALID_USAGE;
    case EINVAL:       return ANVIL_INVALID_USAGE;
    case EFBIG:        return ANVIL_INVALID_USAGE;
    case ENAMETOOLONG: return ANVIL_INVALID_USAGE;

    case ENOENT:  return ANVIL_NOT_EXIST;
    case ENXIO:   return ANVIL_NOT_EXIST;
    case EACCES:  return ANVIL_NOT_EXIST;
    case ENOTDIR: return ANVIL_NOT_EXIST;
    case EISDIR:  return ANVIL_NOT_EXIST;

    case ENOMEM: return ANVIL_ALLOC_FAILED;
    case ENFILE: return ANVIL_ALLOC_FAILED;
    case EMFILE: return ANVIL_ALLOC_FAILED;

    case ENOSPC: return ANVIL_NO_SPACE;
    case EROFS:  return ANVIL_NO_SPACE;
    case EDQUOT: return ANVIL_NO_SPACE;

    case EIO: return ANVIL_ERROR_IO;

    default: return ANVIL_UNKNOWN;
    }
}

/** Creates an error message and error result from errno and the provided message. */
anvil_result anvil_set_errno(const error_t err, const char *format, ...) {
    strncpy(__anvil_error_message_value, strerror(err), MAX_ERROR_MSG);
    const size_t errno_msg_size = strnlen(__anvil_error_message_value, MAX_ERROR_MSG);

    if (errno_msg_size + 2 < MAX_ERROR_MSG && format != nullptr) {
        __anvil_error_message_value[errno_msg_size] = ':';
        __anvil_error_message_value[errno_msg_size + 1] = ' ';

        va_list args;
        va_start(args, format);
        vsnprintf(__anvil_error_message_value + errno_msg_size + 2, MAX_ERROR_MSG - errno_msg_size - 2, format, args);
        va_end(args);
    }
    __anvil_errno_value = anvil_errno_mapping(err);

    #ifndef NDEBUG
    fputs(__anvil_error_message_value, anvil_log);
    fputc('\n', anvil_log);
    anvil_breakpoint();
    #endif
    return __anvil_errno_value;
}

/** Creates an error message from the given result and message. */
anvil_result anvil_set_result(const anvil_result err, const char *format, ...) {
    strncpy(__anvil_error_message_value, anvil_strerror(err), MAX_ERROR_MSG);
    const size_t errno_msg_size = strnlen(__anvil_error_message_value, MAX_ERROR_MSG);

    if (errno_msg_size + 2 < MAX_ERROR_MSG && format != nullptr) {
        __anvil_error_message_value[errno_msg_size] = ':';
        __anvil_error_message_value[errno_msg_size + 1] = ' ';

        va_list args;
        va_start(args, format);
        vsnprintf(__anvil_error_message_value + errno_msg_size + 2, MAX_ERROR_MSG - errno_msg_size - 2, format, args);
        va_end(args);
    }
    __anvil_errno_value = err;

    #ifndef NDEBUG
    fputs(__anvil_error_message_value, anvil_log);
    fputc('\n', anvil_log);
    anvil_breakpoint();
    #endif
    return __anvil_errno_value;
}
