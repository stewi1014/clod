project(cli LANGUAGES C)

set(CLOD_SOURCES
    src/main.c
)

add_executable(cli ${CLOD_SOURCES})
target_link_libraries(cli PRIVATE clod)
