cmake_minimum_required(VERSION 3.25)
project(snemu
        VERSION 0.1.0
        DESCRIPTION "Out-of-tree DPDK net PMD emulating NIC hardware offloads"
        LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CompilerOptions)
include(Sanitizers)

option(SNEMU_BOOTSTRAP_DPDK
       "Build DPDK from third_party/dpdk with -Denable_driver_sdk=true" OFF)
if (SNEMU_BOOTSTRAP_DPDK)
  include(BootstrapDpdk)
endif ()

find_package(DPDK)

find_package(GTest QUIET)
if (NOT GTest_FOUND)
  include(SnemuFetchGTest)
endif ()

enable_testing()

if (NOT DPDK_FOUND)
  return()
endif ()

add_subdirectory(src)
add_subdirectory(tests)
