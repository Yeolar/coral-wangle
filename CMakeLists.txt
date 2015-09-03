#
# Copyright (C) 2015, Yeolar
#

# Setup compiler
if(APPLE)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
else()
    set(CMAKE_C_COMPILER gcc-4.8)
    set(CMAKE_CXX_COMPILER g++-4.8)
endif()
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC")

# Define project
project(wangle)

set(VERSION_MAJOR 13)
set(VERSION_MINOR 0)
set(VERSION_MICRO 0)
set(VERSION_PATCH 0)

cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake-scripts")

# http://www.cmake.org/Wiki/CMake_RPATH_handling#Mac_OS_X_and_the_RPATH
if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
        "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()

# Required packages
find_package(Coral REQUIRED)
set(WANGLE_HAVE_CORAL ${CORAL_FOUND})
find_package(Boost REQUIRED COMPONENTS system thread)
set(WANGLE_HAVE_BOOST ${Boost_FOUND})
find_package(GFlags REQUIRED)
set(WANGLE_HAVE_LIBGFLAGS ${GFLAGS_FOUND})
find_package(Glog)
set(WANGLE_HAVE_LIBGLOG ${GLOG_FOUND})
find_package(OpenSSL)
set(WANGLE_HAVE_LIBSSL ${OPENSSL_FOUND})

# Optional packages
find_package(GTest)
set(WANGLE_HAVE_LIBGTEST ${GTEST_FOUND})

# Setup environment
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Link libraries
link_libraries(
    ${CORAL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${GLOG_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

# Check and setup configurations
include(CheckCXXSourceCompiles REQUIRED)

if(GFLAGS_FOUND)
    set(CMAKE_REQUIRED_FLAGS "-std=c++11")
    set(CMAKE_REQUIRED_LIBRARIES "gflags")

    check_cxx_source_compiles("
        #include <gflags/gflags.h>
        void foo() { google::GetArgv(); }
        int main() { return 0; }"
        WANGLE_UNUSUAL_GFLAGS_NAMESPACE)

    if(WANGLE_UNUSUAL_GFLAGS_NAMESPACE)
        set(WANGLE_GFLAGS_NAMESPACE "google")
    else()
        set(WANGLE_GFLAGS_NAMESPACE "gflags")
    endif()
endif()

# Uninstall
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake-scripts/cmake_uninstall.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND}
        -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

include_directories(${PROJECT_SOURCE_DIR})

# Work on all sub directories
add_subdirectory(wangle/acceptor)
add_subdirectory(wangle/bootstrap)
add_subdirectory(wangle/channel)
add_subdirectory(wangle/codec)
add_subdirectory(wangle/concurrent)
add_subdirectory(wangle/service)
add_subdirectory(wangle/ssl)
add_subdirectory(wangle/deprecated/rx)

# Link libraries
add_library(wangle_static STATIC
    $<TARGET_OBJECTS:wangle_acceptor>
    $<TARGET_OBJECTS:wangle_bootstrap>
    $<TARGET_OBJECTS:wangle_channel>
    $<TARGET_OBJECTS:wangle_codec>
    $<TARGET_OBJECTS:wangle_concurrent>
    $<TARGET_OBJECTS:wangle_rx>
    $<TARGET_OBJECTS:wangle_ssl>
)
set_target_properties(wangle_static PROPERTIES OUTPUT_NAME wangle)
add_library(wangle_shared SHARED
    $<TARGET_OBJECTS:wangle_acceptor>
    $<TARGET_OBJECTS:wangle_bootstrap>
    $<TARGET_OBJECTS:wangle_channel>
    $<TARGET_OBJECTS:wangle_codec>
    $<TARGET_OBJECTS:wangle_concurrent>
    $<TARGET_OBJECTS:wangle_rx>
    $<TARGET_OBJECTS:wangle_ssl>
)
set_target_properties(wangle_shared PROPERTIES OUTPUT_NAME wangle)

install(TARGETS wangle_static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE
                GROUP_READ
                WORLD_READ
)
install(TARGETS wangle_shared
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

# Test
if(GTEST_FOUND)
    enable_testing()
endif()

