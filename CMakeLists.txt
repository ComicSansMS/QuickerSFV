cmake_minimum_required(VERSION 3.30)

project(QuickerSFV)

find_package(OpenSSL REQUIRED)

add_library(quicker_sfv STATIC)
target_sources(quicker_sfv
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/lib
    FILES
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5_digest.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/quicker_sfv.hpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/sfv_file.hpp
    PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/md5_digest.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/quicker_sfv.cpp
    ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/sfv_file.cpp
)
if (MSVC)
    target_sources(quicker_sfv PRIVATE ${PROJECT_SOURCE_DIR}/lib/quicker_sfv/quicker_sfv.natvis)
endif ()
target_compile_features(quicker_sfv PRIVATE cxx_std_20)
target_link_libraries(quicker_sfv PUBLIC OpenSSL::Crypto)

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
    target_link_libraries(QuickerSFV PRIVATE UxTheme.lib quicker_sfv)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT QuickerSFV)
endif(MSVC)
