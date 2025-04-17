project('CLOD', 'c', version: '1.0', default_options: ['c_std=c23'])

# Dependencies
libzstd_proj = subproject('libzstd')
libzstd_dep = libzstd_proj.get_variable('libzstd_dep')

liblzma_proj = subproject('liblzma')
liblzma_dep = liblzma_proj.get_variable('lzma_dep')

# Library
libclod_include = include_directories('src/libclod')
libclod_sources = [
    'src/libclod/clod.h',
    'src/libclod/serialise.c',
]
libclod = library(
    'clod',
    libclod_sources,
    version: '1.0',
    soversion: '1',
    dependencies: [libzstd_dep, liblzma_dep],
)
libclod_dep = declare_dependency(
    link_with: libclod,
    include_directories: libclod_include,
)

# Executable
clod_sources = [
    'src/clod/main.c',
]
clod = executable('clod', clod_sources, dependencies: [libclod_dep])