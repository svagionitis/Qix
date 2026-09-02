add_executable(qix_tui
    main.cpp
    TuiRenderer.cpp
    TuiRenderer.h
)

target_link_libraries(qix_tui
    PRIVATE
        qix_core
)

apply_compiler_flags(qix_tui)
