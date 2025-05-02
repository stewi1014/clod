libclod_headers = [
    'include/anvil.h',
    'include/dh.h',
    'include/lod.h',
    'include/nbt.h',
]

libclod_sources = libclod_headers + [
    'src/anvil_chunk.c',
    'src/anvil_world.c',
    'src/anvil_world.h',
    'src/dh_db.c',
    'src/dh_lod.c',
    'src/file.h',
    'src/file.c',
    'src/nbt.c',
    'src/serialise.c',
]

libclod_dh_migrations = [
    '0010-sqlite-createInitialDataTables.sql',
    '0020-sqlite-createFullDataSourceV2Tables.sql',
    '0030-sqlite-changeTableJournaling.sql',
    '0031-sqlite-useSqliteWalJournaling.sql',
    '0040-sqlite-removeRenderCache.sql',
    '0050-sqlite-addApplyToParentIndex.sql',
    '0060-sqlite-createChunkHashTable.sql',
    '0070-sqlite-createBeaconBeamTable.sql',
    '0080-sqlite-addApplyToChildrenColumn.sql',
]

foreach migration : libclod_dh_migrations
    libclod_sources += custom_target(
        migration,
        input: 'src/dh_migrations/' + migration,
        output: migration + '.c',
        command: ['xxd', '-n', migration, '-i', '@INPUT@', '@OUTPUT@'],
    )
endforeach

add_global_arguments('-DUSE_MMAP', language: 'c')

libclod = library(
    'clod',
    libclod_sources,
    version: '1.0.0',
    soversion: '1',
    include_directories: include_directories('include'),
    dependencies: [
        dependency('libdeflate'),
        dependency('liblz4'),
        dependency('libzstd'),
        dependency('liblzma'),
        dependency('sqlite3'),
        dependency('libpq'),
    ],
)

libclod_dep = declare_dependency(
    link_with: libclod,
    include_directories: include_directories('include'),
)

install_headers(libclod_headers)

# tests
test_cases = [
    'open_world',
    'open_zlib_region',
    'parse_nbt',
    'read_chunk_sections_benchmark',
    'read_chunk_sections',
    'write_dh_database',
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