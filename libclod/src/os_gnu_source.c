#include "os.h"

#ifdef OS_HAS_GNU_SOURCE

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdarg.h>
#include <sys/mman.h>

void *os_mremap (void *old_address, size_t old_size,
    const size_t new_size, int flags, ... /* void *new_address */) {

    int actual_flags = 0;
    if (OS_MREMAP_FIXED) actual_flags |= MREMAP_FIXED;
    if (OS_MREMAP_MAYMOVE) actual_flags |= MREMAP_MAYMOVE;
    if (OS_MREMAP_DONTUNMAP) actual_flags |= MREMAP_DONTUNMAP;

    if (flags & MREMAP_FIXED) {
        va_list va = nullptr;
        va_start(va, flags);
        void *ret = mremap(old_address, old_size, new_size, actual_flags, va_arg(va, void*));
        va_end(va);
        return ret;
    }

    return mremap(old_address, old_size, new_size, actual_flags);
}

#endif
