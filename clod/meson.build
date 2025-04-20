project('CLOD', 'c', version: '1.0', default_options: ['c_std=c23'])

# Sources
libclod_sources = [
    'src/libclod/clod.h',
    'src/libclod/serialise.c',
    'src/libclod/store.c',
]

libregion_sources = [
    'src/libregion/region.c',
    'src/libregion/region.h',
]

clod_sources = [
    'src/clod/main.c',
]

# Dependencies
liblz4_proj = subproject('liblz4')
liblz4_dep = liblz4_proj.get_variable('liblz4_dep')

libzstd_proj = subproject('libzstd')
libzstd_dep = libzstd_proj.get_variable('libzstd_dep')

liblzma_proj = subproject('liblzma')
liblzma_dep = liblzma_proj.get_variable('lzma_dep')

sqlite3_proj = subproject('sqlite3')
sqlite3_dep = sqlite3_proj.get_variable('sqlite3_dep')

zlib_proj = subproject('zlib')
zlib_dep = zlib_proj.get_variable('zlib_dep')

# LIBCLOD
libclod_include = include_directories('src/libclod')
libclod = library(
    'clod',
    libclod_sources,
    version: '1.0',
    soversion: '1',
    dependencies: [libzstd_dep, liblzma_dep, liblz4_dep, sqlite3_dep],
)
libclod_dep = declare_dependency(
    link_with: libclod,
    include_directories: libclod_include,
)
meson.override_dependency('libclod', libclod_dep)

# LIBREGION
libregion_include = include_directories('src/libregion')
libregion = library(
    'region',
    libregion_sources,
    version: '1.0',
    soversion: '1',
    dependencies: [zlib_dep],
)
libregion_dep = declare_dependency(
    link_with: libregion,
    include_directories: libregion_include,
)
meson.override_dependency('libregion', libregion_dep)

# CLOD
clod = executable('clod', clod_sources, dependencies: [libclod_dep, libregion_dep])