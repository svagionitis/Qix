find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
        if(TARGET PkgConfig::SDL2)
            add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
            set(SDL2_FOUND TRUE)
        endif()
    endif()
endif()

if(SDL2_FOUND OR TARGET SDL2::SDL2)
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

    if(TARGET SDL2::SDL2main)
        target_link_libraries(qix_sdl PRIVATE SDL2::SDL2main)
    endif()

    apply_compiler_flags(qix_sdl)
else()
    message(STATUS "SDL2 not found. Skipping qix_sdl.")
endif()
