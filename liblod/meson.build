liblod_sources = [
    'include/dh.h',
    'include/lod.h',
    'src/serialise.c',
    'src/store.c',
]

liblod = library(
    'lod',
    liblod_sources,
    version: '1.0.0',
    soversion: '1',
    include_directories: include_directories('include'),
    dependencies: [
        dependency('libzstd'),
        dependency('liblzma'),
        dependency('liblz4'),
    ],
)

liblod_dep = declare_dependency(
    link_with: liblod,
    include_directories: include_directories('include'),
)

install_headers('include/lod.h', 'include/dh.h')

# tests
test_cases = []

foreach test_case : test_cases
    test(
        test_case,
        executable(
            test_case,
            'test/' + test_case + '.c',
            dependencies: liblod_dep,
        ),
        workdir: join_paths(meson.current_source_dir(), 'test/data'),
    )
endforeach