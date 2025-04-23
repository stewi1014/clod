libmc_sources = [
    'mc.h',
    'src/nbt.c',
    'src/region.c',
]

libmc = library('mc', libmc_sources, version: '1.0.0', soversion: '1')

libmc_dep = declare_dependency(link_with: libmc, include_directories: include_directories('.'))
install_headers('mc.h')

test(
    'test_libmc',
    executable(
        'test_libmc',
        [
            'test/test_nbt.c',
            'test/test_region.c',
            'test/test.c',
            'test/test.h',
        ],
        dependencies: [libmc_dep, dependency('libdeflate')],
    ),
    workdir: join_paths(meson.current_source_dir(), 'test/data'),
)