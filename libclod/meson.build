libclod_sources = [
    'include/anvil.h',
    'include/dh.h',
    'include/lod.h',
    'include/nbt.h',
    'src/anvil.c',
    'src/dh.c',
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

install_headers(
    'include/anvil.h',
    'include/dh.h',
    'include/lod.h',
    'include/nbt.h',
)

# tests
test_cases = [
    'open_world',
    'open_world_benchmark',
    'open_zlib_region',
    'parse_nbt',
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