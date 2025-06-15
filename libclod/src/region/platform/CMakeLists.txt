target_sources(clod PRIVATE
    platform.h
)

if (UNIX)
    add_subdirectory(unix)
endif ()