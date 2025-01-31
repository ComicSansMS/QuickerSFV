cmake_minimum_required(VERSION 3.30)

project(QuickerSFV)

if(MSVC)
    add_executable(QuickerSFV WIN32)
    target_sources(QuickerSFV
        PRIVATE
        ${PROJECT_SOURCE_DIR}/QuickerSFV.cpp
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
    target_link_libraries(QuickerSFV PRIVATE UxTheme.lib)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT QuickerSFV)
endif(MSVC)
