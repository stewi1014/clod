liblod_sources = [
    'lod.h',
    'src/serialise.c',
    'src/store.c',
]

liblod = library(
    'lod',
    liblod_sources,
    version: '1.0.0',
    soversion: '1',
    dependencies: [
        dependency('libzstd'),
        dependency('liblzma'),
        dependency('liblz4'),
    ],
)

liblod_dep = declare_dependency(link_with: liblod, include_directories: include_directories('.'))
install_headers('lod.h')