find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(ALSA QUIET alsa)
endif()
if(NOT ALSA_FOUND)
    find_package(ALSA QUIET)
endif()

add_executable(qix_tui
    main.cpp
    TuiRenderer.cpp
    TuiRenderer.h
    TuiAudio.cpp
    TuiAudio.h
)

target_link_libraries(qix_tui
    PRIVATE
        qix_core
)

if(ALSA_FOUND)
    target_compile_definitions(qix_tui PRIVATE QIX_HAS_ALSA=1)
    if(TARGET PkgConfig::ALSA)
        target_link_libraries(qix_tui PRIVATE PkgConfig::ALSA)
    elseif(TARGET ALSA::ALSA)
        target_link_libraries(qix_tui PRIVATE ALSA::ALSA)
    else()
        target_link_libraries(qix_tui PRIVATE ${ALSA_LIBRARIES})
        target_include_directories(qix_tui PRIVATE ${ALSA_INCLUDE_DIRS})
    endif()
endif()

apply_compiler_flags(qix_tui)
