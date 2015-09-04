#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_RX_TEST_SRCS
    RxTest.cpp
)

foreach(test_src ${WANGLE_RX_TEST_SRCS})
    get_filename_component(test ${test_src} NAME_WE)
    add_executable(${test} ${test_src})
    target_link_libraries(${test}
        ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARIES} wangle_static)
    add_test(wangle_${test} ${test} CONFIGURATIONS Debug)
endforeach()


set(WANGLE_RX_BENCHMARK_SRCS
    RxBenchmark.cpp
)

foreach(bench_src ${WANGLE_RX_BENCHMARK_SRCS})
    get_filename_component(bench ${bench_src} NAME_WE)
    add_executable(${bench} ${bench_src})
    target_link_libraries(${bench} ${GTEST_BOTH_LIBRARIES} wangle_static)
endforeach()

