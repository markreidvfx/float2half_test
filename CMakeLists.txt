cmake_minimum_required(VERSION 3.10)

project(lut3d_perf
    VERSION 1.0.0
    DESCRIPTION "Performance Tests For Various Float Half Conversion"
    HOMEPAGE_URL https://github.com/markreidvfx/float2half_test
    LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(TargetArch)

target_architecture(ARCH)
message(STATUS "ARCH: ${ARCH}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (NOT MSVC)
    set(CMAKE_C_FLAGS "-Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG "-g")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
endif()

add_subdirectory(src)

enable_testing()
add_test(NAME float2half COMMAND float2half ${CMAKE_CURRENT_SOURCE_DIR}/float2half_result.csv)
add_test(NAME half2float COMMAND half2float)