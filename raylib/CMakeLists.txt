find_package(raylib 4.5 QUIET)
if(NOT raylib_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(RAYLIB QUIET raylib)
    endif()
endif()

if(raylib_FOUND OR RAYLIB_FOUND)
    add_executable(qix_raylib
        main.cpp
        RaylibApp.cpp
        RaylibApp.h
        RaylibRenderer.cpp
        RaylibRenderer.h
    )

    if(raylib_FOUND)
        target_link_libraries(qix_raylib
            PRIVATE
                qix_core
                raylib
        )
    else()
        target_include_directories(qix_raylib PRIVATE ${RAYLIB_INCLUDE_DIRS})
        target_link_libraries(qix_raylib
            PRIVATE
                qix_core
                ${RAYLIB_LIBRARIES}
        )
    endif()

    find_package(Threads REQUIRED)
    target_link_libraries(qix_raylib PRIVATE Threads::Threads m dl)

    apply_compiler_flags(qix_raylib)
else()
    message(STATUS "raylib not found. Install libraylib-dev (>=4.5) to build qix_raylib.")
endif()
