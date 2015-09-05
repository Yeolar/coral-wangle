#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_CHANNEL_BROADCAST_TEST_SRCS
    BroadcastHandlerTest.cpp
    BroadcastPoolTest.cpp
    ObservingHandlerTest.cpp
)

foreach(test_src ${WANGLE_CHANNEL_BROADCAST_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test wangle_channel_broadcast_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test}
        ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARIES} wangle_static)
    add_test(${test} ${test} CONFIGURATIONS Debug)
endforeach()

