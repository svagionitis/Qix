cmake_minimum_required(VERSION 3.20)

project(qix VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Default build type to Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Options
option(BUILD_TESTS "Build GoogleTest unit test suites" ON)
option(BUILD_TUI "Build the terminal (ncurses/console) client" ON)
option(BUILD_GUI "Build the desktop graphical client" ON)
option(BUILD_SDL "Build the SDL graphical client" ON)
option(BUILD_BENCHMARKS "Build the performance benchmark suite" ON)

# Compiler flags
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerFlags)

# Core library
add_subdirectory(lib)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Terminal Client
if(BUILD_TUI)
    add_subdirectory(tui)
endif()

# GUI Client
if(BUILD_GUI)
    add_subdirectory(gui)
endif()

# SDL Client
if(BUILD_SDL)
    add_subdirectory(sdl)
endif()

# Clang-Format Target
find_program(CLANG_FORMAT_BIN NAMES clang-format clang-format-15 clang-format-14 clang-format-12)
if(CLANG_FORMAT_BIN)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tui/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sdl/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sdl/*.cpp
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_BIN} -i -style=file ${ALL_SOURCE_FILES}
        COMMENT "Applying clang-format..."
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_BIN} --dry-run --Werror -style=file ${ALL_SOURCE_FILES}
        COMMENT "Checking clang-format compliance..."
    )
endif()
