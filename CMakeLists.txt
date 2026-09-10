cmake_minimum_required(VERSION 3.20)

# Auto-detect vcpkg toolchain if not explicitly provided
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    elseif(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    endif()
endif()

project(qix VERSION 1.0.0 LANGUAGES CXX)

# Emscripten configuration for distro-packaged Emscripten with read-only FROZEN_CACHE
if(EMSCRIPTEN)
    if(NOT DEFINED ENV{EM_CONFIG} OR "$ENV{EM_CONFIG}" STREQUAL "/usr/share/emscripten/.emscripten")
        if(EXISTS "$ENV{HOME}/.emscripten")
            set(QIX_EM_CONFIG "$ENV{HOME}/.emscripten")
        elseif(EXISTS "/usr/share/emscripten/.emscripten")
            set(QIX_EM_CONFIG "${CMAKE_BINARY_DIR}/.emscripten")
            file(READ "/usr/share/emscripten/.emscripten" _EM_CONF_CONTENT)
            string(REPLACE "FROZEN_CACHE = True" "FROZEN_CACHE = False" _EM_CONF_CONTENT "${_EM_CONF_CONTENT}")
            string(APPEND _EM_CONF_CONTENT "\nimport os\nCACHE = os.path.expanduser('~/.emscripten_cache')\n")
            file(WRITE "${QIX_EM_CONFIG}" "${_EM_CONF_CONTENT}")
        endif()
    else()
        set(QIX_EM_CONFIG "$ENV{EM_CONFIG}")
    endif()
    if(QIX_EM_CONFIG)
        add_compile_options("--em-config" "${QIX_EM_CONFIG}")
        add_link_options("--em-config" "${QIX_EM_CONFIG}")
    endif()
endif()

# Auto-detect installed Qt on Windows
if(WIN32)
    file(GLOB QT_CANDIDATE_PATHS "C:/Qt/6.*/msvc2019_64" "C:/Qt/6.*/msvc2022_64" "C:/Qt/5.*/msvc2019_64")
    if(QT_CANDIDATE_PATHS)
        list(GET QT_CANDIDATE_PATHS -1 LATEST_QT_PATH)
        if(NOT "${LATEST_QT_PATH}" IN_LIST CMAKE_PREFIX_PATH)
            list(APPEND CMAKE_PREFIX_PATH "${LATEST_QT_PATH}")
            message(STATUS "Auto-detected Qt at: ${LATEST_QT_PATH}")
        endif()
    endif()
endif()

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
if(EMSCRIPTEN)
    option(BUILD_TESTS "Build GoogleTest unit test suites" OFF)
    option(BUILD_TUI "Build the terminal (ncurses/console) client" OFF)
    option(BUILD_QT "Build the Qt desktop graphical client" OFF)
    option(BUILD_SDL "Build the SDL graphical client" ON)
    option(BUILD_RAYLIB "Build the Raylib graphical client" OFF)
    option(BUILD_BENCHMARKS "Build the performance benchmark suite" OFF)
else()
    option(BUILD_TESTS "Build GoogleTest unit test suites" ON)
    option(BUILD_TUI "Build the terminal (ncurses/console) client" ON)
    option(BUILD_QT "Build the Qt desktop graphical client" ON)
    option(BUILD_SDL "Build the SDL graphical client" ON)
    option(BUILD_RAYLIB "Build the Raylib graphical client" ON)
    option(BUILD_BENCHMARKS "Build the performance benchmark suite" ON)
endif()
option(QIX_DEFAULT_CLASSIC_MODE "Default game mode to Classic (1981 arcade rules)" ON)

if(QIX_DEFAULT_CLASSIC_MODE)
    add_compile_definitions(QIX_DEFAULT_CLASSIC_MODE=1)
else()
    add_compile_definitions(QIX_DEFAULT_CLASSIC_MODE=0)
endif()

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

# Qt Client
if(BUILD_QT)
    add_subdirectory(qt)
endif()

# SDL Client
if(BUILD_SDL)
    add_subdirectory(sdl)
endif()

# Raylib Client
if(BUILD_RAYLIB)
    add_subdirectory(raylib)
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
        ${CMAKE_CURRENT_SOURCE_DIR}/qt/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/qt/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sdl/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/sdl/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/raylib/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/raylib/*.cpp
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
