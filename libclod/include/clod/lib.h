/**
 * @file lib.h
 * @internal
 */
#ifndef CLOD_LIB_H
#define CLOD_LIB_H

#if defined(__GNUC__)
#define CLOD_API __attribute__((visibility("default")))
#define CLOD_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#define CLOD_USE_RETURN __attribute__((warn_unused_result))
#define CLOD_CONST __attribute__((const))
#elif defined(_WIN32) || defined(__CYGWIN__)
#define CLOD_API __declspec(dllexport)
#define CLOD_NONNULL(...)
#define CLOD_USE_RETURN
#define CLOD_CONST
#else
#define CLOD_API
#define CLOD_NONNULL(...)
#define CLOD_USE_RETURN
#define CLOD_CONST
#endif

#endif
