# libclod: the shared library

set(LIBCLOD_HEADERS
    include/anvil.h
    include/dh.h
    include/map.h
    include/nbt.h
)

set(LIBCLOD_SOURCES
    src/anvil/anvil.c
    src/anvil/error.c
    src/anvil/file.c
    src/anvil/filename.c
    src/anvil/opts.c
    src/nbt/nbt.c
    src/map.c
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

add_library(libclod SHARED ${LIBCLOD_SOURCES} ${LIBCLOD_HEADERS})

target_include_directories(libclod
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(libclod PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER "${LIBCLOD_HEADERS}"
)

# Example: link with system libraries if needed
# target_link_libraries(libclod PUBLIC z m)

install(TARGETS libclod
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
