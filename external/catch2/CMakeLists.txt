cmake_minimum_required(VERSION 3.30)

project(Catch2)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/external/catch/catch.hpp)
    file(
        DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v2.13.10/catch.hpp 
        ${PROJECT_BINARY_DIR}/external/catch/catch.hpp
        EXPECTED_HASH SHA256=3725c0f0a75f376a5005dde31ead0feb8f7da7507644c201b814443de8355170
    )
endif()
add_library(Catch2 STATIC ${PROJECT_SOURCE_DIR}/catch_main.cpp)
target_compile_features(Catch2 PRIVATE cxx_std_20)
target_include_directories(Catch2 PUBLIC ${PROJECT_BINARY_DIR}/external/catch)
