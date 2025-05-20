libclod_headers = [
    'include/anvil.h',
    'include/dh.h',
    'include/nbt.h',
]

libclod_sources = libclod_headers + [
    'generated/index.h',
    'generated/index.c',
    'src/anvil/coord.h',
    'src/anvil/dir.c',
    'src/anvil/dir.h',
    'src/anvil/error.c',
    'src/anvil/error.h',
    'src/anvil/file.c',
    'src/anvil/file.h',
    'src/anvil/filename.c',
    'src/anvil/filename.h',
    'src/anvil/iter.c',
    'src/anvil/iter.h',
    'src/anvil/world.c',
    'src/anvil/world.h',
    'src/buffer.c',
    'src/buffer.h',
    'src/nbt.c'
]

libclod_dependencies = [
    dependency('libdeflate'),
    dependency('liblz4'),
    dependency('liblzma'),
    dependency('sqlite3'),
    m_dep
]

libclod = library(
    'libclod',
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
test_programs = [
    'anvil_iter',
    'anvil_parse_filename'
]

test_dir = meson.current_source_dir() / 'test' / 'resources'

foreach test_case : test_programs
    test(
        test_case,
        executable(
            test_case,
            'test/' + test_case + '.c',
            'test/test.h',
            c_args: ['-DTEST_DIR="' + test_dir + '/"'],
            dependencies: libclod_dep,
        )
    )
endforeach
