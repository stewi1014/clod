#pragma once
#include "anvil.h"

struct anvil_world {
    anvil_allocator alloc;

    #ifdef CLOD_USE_POSIX

    int dir_fd;
    int session_lock_fd;

    #else
    #error not implemented
    #endif
};
