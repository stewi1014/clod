#pragma once
#include "config.h"
#include <anvil.h>

#include "map.h"
#include "mutex.h"
#include "region.h"

#define sizeof_anvil(path) (sizeof(anvil) + strlen(path) + 1)
struct anvil {
    anvil_opts opts;
    mutex mtx;

    map(int64_t, region) regions;
    map(void*, chunk) chunks;

    #if HAVE_UNIX
    int dir_fd;
    #else
    #error not implemented
    #endif

    char path[];
};
