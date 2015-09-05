#
# Copyright (C) 2015, Yeolar
#

set(WANGLE_CODEC_TEST_SRCS
    CodecTest.cpp
)

foreach(test_src ${WANGLE_CODEC_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test wangle_codec_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test}
        ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARIES} wangle_static)
    add_test(${test} ${test} CONFIGURATIONS Debug)
endforeach()

