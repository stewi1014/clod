libmc_sources = [
    'include/nbt.h',
    'include/region.h',
    'src/nbt.c',
    'src/region.c',
]

libmc = library(
    'mc',
    libmc_sources,
    version: '1.0.0',
    soversion: '1',
    include_directories: include_directories('include'),
    dependencies: [dependency('libdeflate'), dependency('liblz4')],
)

libmc_dep = declare_dependency(
    link_with: libmc,
    include_directories: include_directories('include'),
)

install_headers('include/nbt.h', 'include/region.h')

# tests
test_cases = [
    'open_region',
    'parse_nbt',
]

foreach test_case : test_cases
    test(
        test_case,
        executable(
            test_case,
            'test/' + test_case + '.c',
            dependencies: libmc_dep,
        ),
        workdir: join_paths(meson.current_source_dir(), 'test'),
    )
endforeach