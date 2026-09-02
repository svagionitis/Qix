add_library(qix_core STATIC
    CollisionDetector.cpp
    CollisionDetector.h
    Fuse.cpp
    Fuse.h
    IQixGame.h
    Marker.cpp
    Marker.h
    Playfield.cpp
    Playfield.h
    Qix.cpp
    Qix.h
    QixGame.cpp
    QixGame.h
    Sparx.cpp
    Sparx.h
    TerritoryFill.cpp
    TerritoryFill.h
    Types.h
)

target_include_directories(qix_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

apply_compiler_flags(qix_core)
