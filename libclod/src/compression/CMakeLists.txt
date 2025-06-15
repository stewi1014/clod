find_package(PkgConfig REQUIRED)

target_sources(clod PRIVATE
        compress.c
        compression.c
        decompress.c
        compression_libraries.h
)

option(USE_LIBDEFLATE "Use libdeflate for gzip, zlib and deflate support" ON)
if (USE_LIBDEFLATE)
    set(HAVE_LIBDEFLATE 1)
    pkg_check_modules(libdeflate REQUIRED IMPORTED_TARGET libdeflate)
    target_link_libraries(clod PRIVATE PkgConfig::libdeflate)
endif ()

option(USE_LZ4 "Use lz4 to enable lz4/lz4hc compression support" ON)
if (USE_LZ4)
    set(HAVE_LZ4 1)
    pkg_check_modules(LZ4 REQUIRED IMPORTED_TARGET liblz4)
    target_link_libraries(clod PRIVATE PkgConfig::LZ4)
endif ()

option(USE_LZMA "Use liblzma to enable lzma compression support" ON)
if (USE_LZMA)
    set(HAVE_LZMA 1)
    pkg_check_modules(LZMA REQUIRED IMPORTED_TARGET liblzma)
    target_link_libraries(clod PRIVATE PkgConfig::LZMA)
endif ()

option(USE_ZSTD "Use libzstd to enable Zstd compression support" ON)
if (USE_ZSTD)
    set(HAVE_ZSTD 1)
    pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)
    target_link_libraries(clod PRIVATE PkgConfig::ZSTD)
endif ()

configure_file(compression_config.h.in compression_config.h)
target_include_directories(clod PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if (HAVE_LIBDEFLATE)
    libclod_test(libdeflate)
endif ()

if (HAVE_LZ4)
    libclod_test(lz4)
endif ()

if (HAVE_LZMA)
    libclod_test(lzma)
endif ()

if (HAVE_ZSTD)
    libclod_test(zstd)
endif ()
