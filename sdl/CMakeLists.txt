if(EMSCRIPTEN)
    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 INTERFACE IMPORTED)
        target_compile_options(SDL2::SDL2 INTERFACE "-sUSE_SDL=2")
        target_link_options(SDL2::SDL2 INTERFACE "-sUSE_SDL=2")
    endif()
    set(SDL2_FOUND TRUE)
else()
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

    if(EMSCRIPTEN)
        set_target_properties(qix_sdl PROPERTIES
            OUTPUT_NAME "qix"
            SUFFIX ".html"
        )
        target_link_options(qix_sdl PRIVATE
            "-sUSE_SDL=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sINITIAL_MEMORY=67108864"
            "-sENVIRONMENT=web"
            "-sMINIFY_HTML=0"
        )
        if(EXISTS "${CMAKE_SOURCE_DIR}/web/shell.html")
            target_link_options(qix_sdl PRIVATE "--shell-file" "${CMAKE_SOURCE_DIR}/web/shell.html")
        endif()
    endif()

    apply_compiler_flags(qix_sdl)
else()
    message(STATUS "SDL2 not found. Skipping qix_sdl.")
endif()
