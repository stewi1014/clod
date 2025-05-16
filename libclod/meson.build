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
    'src/compress.c',
    'src/compress.h',
    'src/dh_db.c',
    'src/dh_lod.c',
    'src/dh_lod.h',
    'src/dh_lod_generate.c',
    'src/dh_lod_mip.c',
    'src/dh_lod_mip_nxn.c',
    'src/file.h',
    'src/file.c',
    'src/nbt.c',
    'src/serialise.c',
]

libclod_resources = [
    'dh_migrations/0010-sqlite-createInitialDataTables.sql',
    'dh_migrations/0020-sqlite-createFullDataSourceV2Tables.sql',
    'dh_migrations/0030-sqlite-changeTableJournaling.sql',
    'dh_migrations/0031-sqlite-useSqliteWalJournaling.sql',
    'dh_migrations/0040-sqlite-removeRenderCache.sql',
    'dh_migrations/0050-sqlite-addApplyToParentIndex.sql',
    'dh_migrations/0060-sqlite-createChunkHashTable.sql',
    'dh_migrations/0070-sqlite-createBeaconBeamTable.sql',
    'dh_migrations/0080-sqlite-addApplyToChildrenColumn.sql',
    'dh_constants/compression_mode',
    'dh_constants/compression_mode.lz4',
    'dh_constants/compression_mode.lzma',
    'dh_constants/gen_step',
    'dh_constants/gen_step.lz4',
    'dh_constants/gen_step.lzma',
]

foreach resource : libclod_resources
    libclod_sources += custom_target(
        resource.replace('/', '_'),
        input: 'resources/' + resource,
        output: resource.replace('/', '_') + '.c',
        command: [
            'xxd',
            '-n', resource.replace('/', '_'),
            '-i', '@INPUT@',
            '@OUTPUT@',
        ],
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

install_headers(libclod_headers)

# tests
test_cases = [
    'dh_generate_and_store_benchmark',
    'dh_generate_benchmark',
    'dh_generate_example',
    'open_world',
    'open_zlib_region',
    'parse_nbt',
    'read_chunk_sections_benchmark',
    'read_chunk_sections',
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
