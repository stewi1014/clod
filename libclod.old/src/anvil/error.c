#include <anvil.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "anvil.h"

void anvil_log_default(const char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
}

_Thread_local anvil_result anvil_result_value = ANVIL_OK;
_Thread_local char anvil_error_message[MAX_ERROR_MSG];
void (*anvil_log)(const char *msg) = anvil_log_default;

const char *anvil_result_strerror(const anvil_result res) {
    switch (res) {
    case ANVIL_OK: return "Ok";
    case ANVIL_MALFORMED: return "Malformed";
    case ANVIL_IO_ERROR: return "IO Error";
    case ANVIL_INVALID_USAGE: return "Invalid usage";
    case ANVIL_ALLOC_FAILED: return "Allocation failed";
    case ANVIL_NOT_EXIST: return "Does not exist";
    case ANVIL_NO_SPACE: return "No space on device";
    default: return "Unknown";
    }
}

anvil_result errno_mapping(const error_t err) {
    switch (err) {
    case EPERM: return ANVIL_MALFORMED;

    case EBADF:
    case EINVAL:
    case EFBIG:
    case ENAMETOOLONG: return ANVIL_INVALID_USAGE;

    case ENOENT:
    case ENXIO:
    case EACCES:
    case ENOTDIR:
    case EISDIR:  return ANVIL_NOT_EXIST;

    case ENOMEM:
    case ENFILE:
    case EMFILE: return ANVIL_ALLOC_FAILED;

    case ENOSPC:
    case EROFS:
    case EDQUOT: return ANVIL_NO_SPACE;

    case EIO: return ANVIL_IO_ERROR;

    default: return ANVIL_UNKNOWN;
    }
}

void anvil_set_error(const anvil_result res, const char *err_str, const char *format, ...) {
    anvil_result_value = res;

    char *cursor = stpncpy(anvil_error_message, err_str, MAX_ERROR_MSG);
    if (format != nullptr && MAX_ERROR_MSG - (cursor - anvil_error_message) > 3) {
        cursor[0] = ' ';
        cursor[1] = '|';
        cursor[2] = ' ';
        cursor += 3;

        va_list args;
        va_start(args, format);
        vsnprintf(
            cursor,
            MAX_ERROR_MSG - (cursor - anvil_error_message),
            format,
            args
        );
        va_end(args);
    }
    anvil_error_message[MAX_ERROR_MSG - 1] = '\0';

    if (anvil_log) anvil_log(anvil_error_message);

    #ifndef NDEBUG
    anvil_breakpoint();
    #endif
}

/**
 * Get the error value.
 * @return Error result.
 */
anvil_result anvil_error() {
    return anvil_result_value;
}

/**
 * Get a detailed error message.
 * The message is valid until the next
 * @return Error message.
 */
const char *anvil_strerror() {
    if (anvil_result_value == ANVIL_OK) return nullptr;
    return anvil_error_message;
}

/**
 * Clear the error value.
 */
void anvil_clear_error() {
    anvil_result_value = ANVIL_OK;
    anvil_error_message[0] = '\0';
}

/**
 * Set the method where messages are sent.
 * The default logger writes to stderr.
 */
void anvil_set_logger(void (*logger)(const char *msg)) {
    anvil_log = logger;
}
