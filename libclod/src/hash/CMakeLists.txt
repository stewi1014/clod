target_sources(clod PRIVATE
    crc.c
    crc_tables.h
    siphash.c
)

libclod_test(expected_outputs)
libclod_test(streaming_test)
