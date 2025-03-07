option(GENERATE_COVERAGE_INFO "If set, line coverage info will be generated from debug test runs." OFF)
if(GENERATE_COVERAGE_INFO)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(FATAL_ERROR "Coverage collection only supported for Debug builds.")
        endif()
        find_program(GCOVR_COMMAND NAMES gcovr DOC "Command for running gcovr.")
        if(NOT GCOVR_COMMAND)
            message(FATAL_ERROR "gcovr was not found.")
        endif()
        add_compile_options(--coverage)
        add_compile_options(-fprofile-abs-path)
        add_compile_options(-fno-elide-constructors)
        add_compile_options(-fno-default-inline)
        link_libraries(--coverage)
        add_custom_target(coverage_html
            COMMAND ${CMAKE_COMMAND} -E make_directory coverage
            COMMAND ${GCOVR_COMMAND}
                --html-details
                --output coverage/index.html
                --root ${PROJECT_SOURCE_DIR}
                --exclude .*\.t\.cpp
                --exclude .*/test/.*hpp
                --exclude .*/catch/catch\.hpp
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Generating gcovr html coverage report"
        )
    else()
        message(STATUS "Coverage not supported on current compiler")
    endif()
endif()
