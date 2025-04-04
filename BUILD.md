Build Instructions
==================

QuickerSFV can be built on Windows using Microsoft Visual Studio 2022. The core and client support libraries can be built on Linux using clang 19+ or gcc 14+. There is currently no user client for Linux. This document describes the steps for building the full project on Windows.

Prerequisites
-------------

- Microsoft Visual Studio 2022 with Platform toolset v143 (VS 2022 C++ x86/x64 build tools v14.3x).
- Microsoft Windows 11 SDK 10.0.22621.0 (if you get compile errors due to missing min/max macros in gdiplus.h, you need to upgrade your Windows SDK).
- [nasm - The Netwide Assembler](https://www.nasm.us/) version 2.15 or higher.
- CMake version 3.30 or higher.

Build Instructions
------------------

To build the project using CMake:

    $ cd QuickerSFV
    $ cmake -S . -B build -G"Visual Studio 17 2022"
    $ cmake --build build --config Release

Build Options
-------------

The following options can be set in CMake to configure the build.

   - `QUICKER_SFV_BUILD_SELF_CONTAINED` (Default: `OFF`) - Perform a self-contained build. These builds statically link the C++ runtime on Windows, so that the resulting binaries don't require the Visual C++ Redistributable to be installed. The resulting binaries will be slightly bigger and will not have plugin support.
   - `QUICKER_SFV_USE_BUNDLED_OPENSSL` (Default: `ON`) - Use the bundled MD5 implementation instead of searching for a system installation of OpenSSL. If this is set to ON, `nasm` is required for compilation of the MD5 algorithms. If this is set to `OFF`, CMake will attempt to find an installed version of OpenSSL on the system and link against that instead. This option is not available on Linux builds, or if QUICKER_SFV_BUILD_SELF_CONTAINED is set to `ON`.
   - `BUILD_TESTS` (Default: `ON`) - Build the test suite. This requires an internet connection for downloading Catch2. Tests can be executed with CMake's CTest.
   - `BUILD_DOCUMENTATION` (Default: `ON`) - Add support for building the developer documentation. This requires Doxygen (v1.13+) to be installed on the system. It is recommended to also have Graphviz (v12+) installed on the system for building the documentation. Documentation can be generated by building the `generate_docs` target with CMake.
   - `BUILD_WITH_SANITIZERS` (Default: `OFF`) - Build with ASan, UBSan and fuzzing enabled. This option is only available when building with clang.
   - `GENERATE_COVERAGE_INFO` (Default: `OFF`) - Collect coverage information from running the test suite. An HTML coverage report can be generated by building the `coverage_html` target with CMake. This option is only available when building with gcc.
   - `BUILD_PLUGINS` (Default: `ON`) - Enable building of the bundled plugins. This option is not available if `QUICKER_SFV_BUILD_SELF_CONTAINED` is set to `ON`.
   - `BUILD_SHA1_PLUGIN` (Default: `ON`) - Build the SHA1 C plugin. This option is only available if `BUILD_PLUGINS` is `ON`.
   - `BUILD_RAR_PLUGIN` (Default: `ON`) - Build the WinRar C++ plugin. This option is only available if `BUILD_PLUGINS` is `ON`.
