set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets QUIET)

if(QT_FOUND)
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

    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin")
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET qix_gui POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:qix_gui>" --no-translations
                COMMENT "Deploying Qt runtime libraries for qix_gui..."
            )
        endif()
    endif()
else()
    message(STATUS "Qt (Qt5 or Qt6) not found. Skipping qix_gui.")
endif()
