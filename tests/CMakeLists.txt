find_package(GTest REQUIRED)

add_executable(qix_tests
    PlayfieldTest.cpp
    MarkerTest.cpp
    TerritoryFillTest.cpp
    CollisionTest.cpp
    GameEngineTest.cpp
    SpeedConfigTest.cpp
)

target_link_libraries(qix_tests
    PRIVATE
        qix_core
        GTest::gtest
        GTest::gtest_main
)

apply_compiler_flags(qix_tests)

include(GoogleTest)
gtest_discover_tests(qix_tests)
