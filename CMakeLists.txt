cmake_minimum_required(VERSION 3.30)

project(QuickerSFV)

find_package(OpenSSL REQUIRED)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/permissive->)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>)
add_compile_options($<$<CXX_COMPILER_ID:GNU>:-Wall>)
add_compile_options($<$<CXX_COMPILER_ID:GNU>:-pedantic>)
add_compile_options($<$<CXX_COMPILER_ID:GNU>:-Wextra>)

add_subdirectory(external/chromium_zlib)

add_library(quicker_sfv STATIC)
target_sources(quicker_sfv
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/lib
    FILES
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/checksum_file.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/crc32.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/digest.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/error.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/file_io.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/hasher.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5_digest.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5_file.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/quicker_sfv.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/sfv_file.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/string_conversion.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/utf.hpp
    PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/checksum_file.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/crc32.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/digest.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/error.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/file_io.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/hasher.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5_digest.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5_file.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/quicker_sfv.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/sfv_file.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/string_conversion.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/utf.cpp
)
if (MSVC)
    target_sources(quicker_sfv PRIVATE ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/quicker_sfv.natvis)
endif ()
target_compile_features(quicker_sfv PUBLIC cxx_std_20)
target_link_libraries(quicker_sfv PRIVATE OpenSSL::Crypto chromium-zlib)

option(BUILD_TESTS "Determines whether to build tests." ON)
if(BUILD_TESTS)
    enable_testing()

    if(NOT TARGET Catch)
        if(NOT EXISTS ${PROJECT_BINARY_DIR}/external/catch/catch.hpp)
            file(DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v2.13.10/catch.hpp ${PROJECT_BINARY_DIR}/external/catch/catch.hpp)
        endif()
        add_library(Catch STATIC ${PROJECT_SOURCE_DIR}/test/catch_main.cpp)
        target_include_directories(Catch PUBLIC ${PROJECT_BINARY_DIR}/external/catch)
    endif()

    add_executable(quicker_sfv_test)
    target_sources(quicker_sfv_test
        PRIVATE
        ${PROJECT_SOURCE_DIR}/test/utf.t.cpp
    )
    target_link_libraries(quicker_sfv_test PRIVATE quicker_sfv Catch)
    target_compile_options(quicker_sfv_test PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_test(NAME "QuickerSFV Unit Tests" COMMAND quicker_sfv_test)
endif()


if(MSVC)
    add_executable(QuickerSFV WIN32)
    target_sources(QuickerSFV
        PRIVATE
        ${PROJECT_SOURCE_DIR}/QuickerSFV.cpp
        ${PROJECT_SOURCE_DIR}/client_gui/ui/file_dialog.cpp
        PRIVATE
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/client_gui
        FILES
        ${PROJECT_SOURCE_DIR}/client_gui/ui/file_dialog.hpp
        PRIVATE
        ${PROJECT_SOURCE_DIR}/resource/quicker_sfv.rc
        PRIVATE
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/resource
        FILES
        ${PROJECT_SOURCE_DIR}/resource/resource.h
    )
    target_compile_options(QuickerSFV PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )
    target_compile_features(QuickerSFV PRIVATE cxx_std_23)
    target_compile_definitions(QuickerSFV PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(QuickerSFV PRIVATE UxTheme.lib Comctl32.lib quicker_sfv)
    target_link_options(QuickerSFV PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT QuickerSFV)
endif(MSVC)
