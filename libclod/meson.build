libclod_headers = [
    'include/anvil.h',
    'include/dh.h',
    'include/lod.h',
    'include/nbt.h',
]

libclod_sources = libclod_headers + [
    'generated/index.h',
    'generated/index.c',
    'src/anvil_region_dir.c',
    'src/anvil_region_file.c',
    'src/anvil_world.c',
    'src/buffer.h',
    'src/compress.c',
    'src/compress.h',
    'src/dh_db.c',
    'src/dh_lod.c',
    'src/dh_lod.h',
    'src/dh_lod_generate.c',
    'src/dh_lod_mip.c',
    'src/dh_lod_mip_nxn.c',
    'src/nbt.c',
    'src/os.h',
    'src/os_gnu_source.c',
    'src/serialise.c',
]

libclod_dependencies = [
    dependency('libdeflate'),
    dependency('liblz4'),
    dependency('liblzma'),
    dependency('sqlite3'),
]

libclod = library(
    'clod',
    libclod_sources,
    version: '1.0.0',
    soversion: '1',
    include_directories: include_directories('include'),
    dependencies: libclod_dependencies,
)

libclod_dep = declare_dependency(
    link_with: libclod,
    include_directories: include_directories('include'),
)

install_headers(libclod_headers)

# tests
test_cases = [
    'dh_generate_and_store_benchmark',
    'dh_generate_benchmark',
    'dh_generate_example',
    'open_world',
    'open_zlib_region',
    'parse_nbt',
    'read_chunk_sections_benchmark',
    'read_chunk_sections',
]

foreach test_case : test_cases
    test(
        test_case,
        executable(
            test_case,
            'test/' + test_case + '.c',
            dependencies: libclod_dep,
        ),
        workdir: join_paths(meson.current_source_dir(), 'test'),
    )
endforeach
