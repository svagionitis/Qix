find_package(GTest QUIET)

if(GTest_FOUND)
    add_executable(qix_tests
        ArcadeAudioTest.cpp
        BackgroundArtTest.cpp
        ColorPaletteTest.cpp
        PlayfieldTest.cpp
        MarkerTest.cpp
        TerritoryFillTest.cpp
        CollisionTest.cpp
        GameEngineTest.cpp
        GameConfigTest.cpp
        GamePresenterTest.cpp
        HighScoreTableTest.cpp
        ParticleSystemTest.cpp
        SpeedConfigTest.cpp
        TuiInputTest.cpp
        ../tui/TuiRenderer.cpp
    )

    target_include_directories(qix_tests PRIVATE ../tui)

    target_link_libraries(qix_tests
        PRIVATE
            qix_core
            GTest::gtest
            GTest::gtest_main
    )

    apply_compiler_flags(qix_tests)

    include(GoogleTest)
    gtest_discover_tests(qix_tests)
else()
    message(STATUS "GoogleTest not found. Skipping qix_tests.")
endif()
