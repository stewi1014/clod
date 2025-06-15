target_sources(clod PRIVATE
    error.c
    error.h
    file_cache.c
    filename.c
    filename.h
    region_file.c
    region_file.h
    region_impl.h
    region_open.c
    region_read.c
)

add_subdirectory(platform)
