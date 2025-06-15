include(CheckSymbolExists)
include(CheckIncludeFile)

target_sources(clod PRIVATE
        coord.h
        error.c
        error.h
        filename.c
        opts.c
        platform.h
        region_files.c
        region_impl.h
        region_open.c
        sync.h
)

if(UNIX)
    add_compile_definitions(_GNU_SOURCE _FILE_OFFSET_BITS=64)
    set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)

    check_symbol_exists(mremap "sys/mman.h" HAVE_MREMAP)
    check_symbol_exists(statx "sys/stat.h" HAVE_STATX)
    check_include_file("pthread.h" HAVE_PTHREAD)

    include_directories()
    target_include_directories(clod PRIVATE platform/unix)
    target_sources(clod PRIVATE
            platform/unix/directory.c
            platform/unix/directory_iter.c
            platform/unix/file.c
            platform/unix/platform_impl.h
    )
elseif(WIN32)
    message(FATAL_ERROR "Windows support is yet to be added.")
else()
    message(FATAL_ERROR "Unknown target platform.")
endif()

configure_file(region_config.h.in region_config.h)
target_include_directories(clod PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
