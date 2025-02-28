cmake_minimum_required(VERSION 3.30)

project(chromium-zlib)

add_library(chromium-zlib)
target_sources(chromium-zlib
    PRIVATE
    ${PROJECT_SOURCE_DIR}/fast_crc32.cpp
    ${PROJECT_SOURCE_DIR}/crc32_simd_sse42.cpp
    ${PROJECT_SOURCE_DIR}/crc32_simd_avx512.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${PROJECT_SOURCE_DIR}/fast_crc32.hpp
)
target_compile_features(chromium-zlib PRIVATE cxx_std_20)
set_source_files_properties(
    ${PROJECT_SOURCE_DIR}/crc32_simd_sse42.cpp
    PROPERTIES COMPILE_OPTIONS
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-msse4.2;-mpclmul>"
)
set_source_files_properties(
    ${PROJECT_SOURCE_DIR}/crc32_simd_avx512.cpp
    PROPERTIES COMPILE_OPTIONS
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-msse4.2;-mpclmul;-mavx512f;-mvpclmulqdq>"
)
target_compile_options(chromium-zlib PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -pedantic>
)
