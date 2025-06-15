project(libanvil LANGUAGES C)
include(CheckSymbolExists)
include(CheckTypeSize)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_C_FLAGS "-mavx512bw -mavx2")

option(DISABLE_CONCURRENCY "Disable concurrency support" 0)

if(WIN32)

        set(HAVE_WINDOWS 1)

elseif(APPLE)

        add_compile_definitions(_DARWIN_C_SOURCE)
        set(CMAKE_REQUIRED_DEFINITIONS -D_DARWIN_C_SOURCE)

        set(HAVE_UNIX 1)

elseif(UNIX)

        add_compile_definitions(_GNU_SOURCE)
        set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)

        set(HAVE_UNIX 1)
        check_symbol_exists(mremap "sys/mman.h" HAVE_MREMAP)
        check_symbol_exists(F_OFD_SETLK "fcntl.h" HAVE_OFD_LOCKS)

endif()

check_symbol_exists(pthread_mutex_init "pthread.h" HAVE_PTHREAD_MUTEX)
check_symbol_exists(mtx_init "threads.h" HAVE_THREADS_MUTEX)

set(LIBANVIL_HEADERS
        include/anvil.h
        include/map.h
)

set(LIBANVIL_SOURCES
        src/anvil.c
        src/anvil.h
        src/chunk.c
        src/chunk.h
        src/error.c
        src/error.h
        src/filename.c
        src/filename.h
        src/map.c
        src/mutex.h
        src/region.c
        src/region.h
        src/vec.h
)

configure_file(config.h config.h)

add_library(libanvil
        SHARED ${LIBANVIL_SOURCES} ${LIBANVIL_HEADERS}
)

target_include_directories(libanvil
        PUBLIC include
        SYSTEM PRIVATE /usr/include
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(libanvil
        PROPERTIES PUBLIC_HEADER "${LIBANVIL_HEADERS}"
)

install(TARGETS libanvil
        PUBLIC_HEADER DESTINATION include
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)

#=========#
# Testing #
#=========#

include(CTest)

function(libanvil_test name)
        add_executable(${name} test/${name}.c)
        target_link_libraries(${name} PRIVATE libanvil)
        target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
        add_test(NAME ${name} COMMAND ${name})
endfunction()

libanvil_test(map_test)
if (HAVE_OFD_LOCKS)
        libanvil_test(ofd_lock)
endif()
