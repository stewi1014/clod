target_sources(clod PRIVATE
    table.c
)

libclod_test(table_set_get_many)
libclod_test(table_simple_all_methods)

target_link_libraries(clod PRIVATE m)

include(CheckSymbolExists)
include(CheckIncludeFile)

set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(mremap "sys/mman.h" HAVE_MREMAP)
check_symbol_exists(statx "sys/stat.h" HAVE_STATX)
check_include_file("pthread.h" HAVE_PTHREAD)
check_symbol_exists(clock_gettime "time.h" HAVE_CLOCK_GETTIME)

execute_process(COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(clod_config.h.in clod_config.h)
target_include_directories(clod PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

add_subdirectory(compression)
add_subdirectory(hash)
add_subdirectory(nbt)
add_subdirectory(region)
