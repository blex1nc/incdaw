cmake_minimum_required(VERSION 3.28)

project(INCDAW
    VERSION 0.1.0
    DESCRIPTION "INCDAW — professional digital audio workstation"
    LANGUAGES C CXX)

# ── Global settings ───────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "")
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
endif()

# ── Warnings: strict everywhere, errors in CI ─────────────────────────────────
add_library(incdaw_warnings INTERFACE)
target_compile_options(incdaw_warnings INTERFACE
    $<$<COMPILE_LANGUAGE:C,CXX,OBJCXX>:
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual
        -Wcast-align -Wunused -Wconversion -Wsign-conversion
        -Wdouble-promotion -Wformat=2
    >)

option(INCDAW_WERROR "Treat warnings as errors" ON)
if(INCDAW_WERROR)
    target_compile_options(incdaw_warnings INTERFACE
        $<$<COMPILE_LANGUAGE:C,CXX,OBJCXX>:-Werror>)
endif()

# ── Realtime-safety guard (debug builds only, armed in Phase 2) ───────────────
option(INCDAW_REALTIME_GUARD "Assert on allocation/locking in the audio thread" ON)

# Single source of truth for the version, consumed by tools/make-dmg.sh.
file(WRITE "${CMAKE_BINARY_DIR}/VERSION" "${PROJECT_VERSION}")

# ── Layers ────────────────────────────────────────────────────────────────────
add_subdirectory(src)

# ── Diagnostic tools ──────────────────────────────────────────────────────────
add_executable(incdaw-audiocheck tools/audiocheck/main.cpp)
target_link_libraries(incdaw-audiocheck PRIVATE incdaw_engine incdaw_warnings)

# ── Tests ─────────────────────────────────────────────────────────────────────
option(INCDAW_BUILD_TESTS "Build the test suite" ON)
if(INCDAW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
