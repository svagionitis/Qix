find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
endif()

add_executable(qix_sdl
    main.cpp
    SdlApp.cpp
    SdlApp.h
    SdlRenderer.cpp
    SdlRenderer.h
    BitmapFont.h
)

target_link_libraries(qix_sdl
    PRIVATE
        qix_core
        SDL2::SDL2
)

apply_compiler_flags(qix_sdl)
