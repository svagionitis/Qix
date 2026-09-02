add_executable(qix_benchmarks
    main.cpp
)

target_link_libraries(qix_benchmarks
    PRIVATE
        qix_core
)

apply_compiler_flags(qix_benchmarks)
