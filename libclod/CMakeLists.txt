project(libclod LANGUAGES C)

add_library(clod SHARED)

set_target_properties(clod PROPERTIES SOVERSION 1)
target_include_directories(clod PUBLIC include)
target_compile_definitions(clod PRIVATE _GNU_SOURCE _FILE_OFFSET_BITS=64)
set_target_properties(clod PROPERTIES
    CMAKE_C_STANDARD 23
    CMAKE_C_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_options(clod PRIVATE "-std=c23" "-mavx2")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(clod PRIVATE "-Wall" "-Wextra" "-Wpedantic")
    if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(clod PRIVATE "-Wshadow" "-Wconversion" "-Wdouble-promotion" "-Wnull-dereference" "-Werror")
        target_compile_options(clod PRIVATE "-fsanitize=address,undefined")
        target_link_options(clod PRIVATE "-fsanitize=address,undefined")
    elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(clod PRIVATE "/W4" "/WX")
    endif()
endif()

function(libclod_test name)
    string(REPLACE "/src" "/test" test_dir ${CMAKE_CURRENT_SOURCE_DIR})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}")
    if(rel_path STREQUAL "." OR rel_path STREQUAL "")
        set(test_name "${name}")
    else()
        string(REPLACE "/" "_" path "${rel_path}")
        set(test_name "${path}_${name}")
    endif()

    add_executable(test_${test_name} ${test_dir}/${name}.c)
    target_include_directories(test_${test_name} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/test)
    target_link_libraries(test_${test_name} PRIVATE clod)
    add_test(NAME ${test_name} COMMAND test_${test_name})
endfunction()

add_subdirectory(src)
