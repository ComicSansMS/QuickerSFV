cmake_minimum_required(VERSION 3.30)

project(openssl-crypto)

find_program(NASM_EXECUTABLE NAMES nasm DOC "NASM (Netwide Assembler) executable")

if(NASM_EXECUTABLE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(BUILD_64_BIT ON)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(BUILD_64_BIT OFF)
    endif()

    if(BUILD_64_BIT)
        add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/generated/md5_asm.obj
            COMMAND ${NASM_EXECUTABLE} -f win64 ${PROJECT_SOURCE_DIR}/crypto/md5/md5-x86_64.asm -o ${PROJECT_BINARY_DIR}/generated/md5_asm.obj
            DEPENDS ${PROJECT_SOURCE_DIR}/crypto/md5/md5-x86_64.asm
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/generated/md5_asm.obj
            COMMAND ${NASM_EXECUTABLE} -f win32 ${PROJECT_SOURCE_DIR}/crypto/md5/md5-586.S -o ${PROJECT_BINARY_DIR}/generated/md5_asm.obj
            DEPENDS ${PROJECT_SOURCE_DIR}/crypto/md5/md5-x86_64.asm
            VERBATIM
        )
    endif()
endif()

add_library(openssl-crypto)
target_sources(openssl-crypto
    PRIVATE
    ${PROJECT_SOURCE_DIR}/crypto/md5/md5_dgst.c
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES
    ${PROJECT_SOURCE_DIR}/crypto/md5/md5_local.h
    ${PROJECT_SOURCE_DIR}/crypto/md32_common.h
    ${PROJECT_SOURCE_DIR}/openssl/md5.h
)
if(NASM_EXECUTABLE)
    target_sources(openssl-crypto PRIVATE ${PROJECT_BINARY_DIR}/generated/md5_asm.obj)
    target_compile_definitions(openssl-crypto PRIVATE MD5_ASM)
endif()

add_library(OpenSSL::Crypto ALIAS openssl-crypto)
