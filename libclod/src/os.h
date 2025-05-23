#pragma once
#include <stddef.h>

#if defined(__linux__) || defined(__ANDROID__)

    #define POSIX

    #define OS_HAS_GNU_SOURCE

    #define OS_MREMAP_MAYMOVE 1<<1
    #define OS_MREMAP_FIXED 1<<2
    #define OS_MREMAP_DONTUNMAP 1<<3

    void *os_mremap(void *old_address, size_t old_size,
                size_t new_size, int flags, ... /* void *new_address */);

    #define PATH_SEP "/"

#elif defined(__FreeBSD__)   || \
      defined(__NetBSD__)    || \
      defined(__OpenBSD__)   || \
      defined(__bsdi__)      || \
      defined(__DragonFly__)

    #define POSIX

    #define PATH_SEP "/"

#elif defined(__CYGWIN__)

    #define POSIX

    #define PATH_SEP "/"

#elif defined(__APPLE__) && defined(__MACH__)

    #define POSIX

    #define PATH_SEP "/"

#elif defined(_WIN64)

    #define WINDOWS

    #define PATH_SEP "\\"

#endif
