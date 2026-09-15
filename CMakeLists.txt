cmake_minimum_required(VERSION 3.25)

# Align all targets to use the DLL CRT (/MD or /MDd) — prevents LNK4098.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

project(AccessOS
    VERSION 0.1.0
    DESCRIPTION "Native Windows accessibility platform"
    LANGUAGES CXX
)

# ─── C++ standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Windows target ──────────────────────────────────────────────────────────
# Require Windows 10 1809 (build 17763) minimum — UIA and modern APIs.
add_compile_definitions(
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

# ─── Compiler warnings ───────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(
        /W4         # Warning level 4
        /WX         # Warnings as errors
        /permissive- # Conformance mode
        /Zc:__cplusplus
        /utf-8
    )
endif()

# ─── Output directories ──────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ─── Version header generation ───────────────────────────────────────────────
configure_file(
    "${CMAKE_SOURCE_DIR}/src/Core/Version.h.in"
    "${CMAKE_BINARY_DIR}/generated/Version.h"
    @ONLY
)

# ─── SQLite amalgamation (vendored) ──────────────────────────────────────────
# Single-file C amalgamation bundled in third_party/sqlite/.
# No network access required. Version: 3.45.2.

add_library(sqlite3_lib STATIC
    "${CMAKE_SOURCE_DIR}/third_party/sqlite/sqlite3.c"
)
target_include_directories(sqlite3_lib PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/sqlite"
)
# Suppress warnings in third-party code and align CRT with the rest of the build.
if(MSVC)
    target_compile_options(sqlite3_lib PRIVATE /W0)
    # Use the same CRT as the rest of the build (debug = /MDd, release = /MD).
    set_target_properties(sqlite3_lib PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

# ─── GoogleTest ──────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# Prevent GoogleTest from overriding compiler/linker settings on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# ─── Subdirectories ──────────────────────────────────────────────────────────
add_subdirectory(src/Core)
add_subdirectory(Tests)
