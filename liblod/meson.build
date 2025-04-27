liblod_sources = [
    'include/dh.h',
    'include/lod.h',
    'src/dh.c',
    'src/serialise.c',
    'src/store.c',
]

liblod_dh_migrations = [
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

foreach migration : liblod_dh_migrations
    liblod_sources += custom_target(
        migration,
        input: 'src/dh_migrations/' + migration,
        output: migration + '.c',
        command: ['xxd', '-n', migration, '-i', '@INPUT@', '@OUTPUT@'],
    )
endforeach

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
        dependency('sqlite3'),
    ],
)

liblod_dep = declare_dependency(
    link_with: liblod,
    include_directories: include_directories('include'),
)

install_headers('include/lod.h', 'include/dh.h')

# tests
test_cases = ['write_dh_database']

foreach test_case : test_cases
    test(
        test_case,
        executable(
            test_case,
            'test/' + test_case + '.c',
            dependencies: liblod_dep,
        ),
        workdir: join_paths(meson.current_source_dir(), 'test'),
    )
endforeach