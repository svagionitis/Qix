set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets REQUIRED)

add_executable(qix_gui
    main.cpp
    MainWindow.cpp
    MainWindow.h
    QixCanvas.cpp
    QixCanvas.h
)

target_link_libraries(qix_gui
    PRIVATE
        qix_core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
)

apply_compiler_flags(qix_gui)
