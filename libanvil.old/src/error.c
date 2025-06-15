#include "error.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void anvil_log_default(const char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
}

_Thread_local anvil_result error_value = ANVIL_OK;
_Thread_local char error_message[MAX_ERROR_MSG];
void (*error_log)(const char *msg) = anvil_log_default;

const char *strresult(const anvil_result res) {
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

anvil_result get_result(const error_t err) {
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

anvil_result error(const anvil_result res, const char *err_str, const char *format, ...) {
    error_value = res;

    char *cursor = stpncpy(error_message, err_str, MAX_ERROR_MSG);
    if (format != nullptr && MAX_ERROR_MSG - (cursor - error_message) > 3) {
        cursor[0] = ' ';
        cursor[1] = '|';
        cursor[2] = ' ';
        cursor += 3;

        va_list args;
        va_start(args, format);
        vsnprintf(
            cursor,
            MAX_ERROR_MSG - (size_t)(cursor - error_message),
            format,
            args
        );
        va_end(args);
    }
    error_message[MAX_ERROR_MSG - 1] = '\0';

    if (error_log) error_log(error_message);

    #ifndef NDEBUG
    anvil_breakpoint();
    #endif

    return res;
}

void pop_error(anvil_result *res, char *msg) {
    *res = error_value; error_value = ANVIL_OK;
    memcpy(msg, error_message, MAX_ERROR_MSG);
    memset(error_message, 0, MAX_ERROR_MSG);
}

void push_error(const anvil_result res, char *msg) {
    error_value = res;
    memcpy(error_message, msg, MAX_ERROR_MSG);
    memset(msg, 0, MAX_ERROR_MSG);
}

/**
 * Get the error value.
 * @return Error result.
 */
anvil_result anvil_error() {
    return error_value;
}

/**
 * Get a detailed error message.
 * The message is valid until the next
 * @return Error message.
 */
const char *anvil_strerror() {
    if (error_value == ANVIL_OK) return nullptr;
    return error_message;
}

/**
 * Clear the error value.
 */
void anvil_clear_error() {
    error_value = ANVIL_OK;
    error_message[0] = '\0';
}

/**
 * Set the method where messages are sent.
 * The default logger writes to stderr.
 */
void anvil_set_logger(void (*logger)(const char *msg)) {
    error_log = logger;
}
