libmc_sources = [
    'mc.h',
    'src/file.c',
    'src/file.h',
    'src/nbt.c',
    'src/nbt.h',
]

libmc = library(
    'mc',
    libmc_sources,
    version: '1.0.0',
    soversion: '1',
    dependencies: [
        dependency('zlib'),
    ],
)

libmc_dep = declare_dependency(link_with: libmc, include_directories: include_directories('.'))
install_headers('mc.h')