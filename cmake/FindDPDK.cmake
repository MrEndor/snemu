include_guard(GLOBAL)

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_DPDK QUIET IMPORTED_TARGET GLOBAL libdpdk)

find_package_handle_standard_args(DPDK
        REQUIRED_VARS PC_DPDK_INCLUDE_DIRS PC_DPDK_LIBRARIES
        VERSION_VAR   PC_DPDK_VERSION)

if (DPDK_FOUND AND NOT TARGET DPDK::dpdk)
  add_library(DPDK::dpdk ALIAS PkgConfig::PC_DPDK)
  message(STATUS "DPDK found. Version: ${PC_DPDK_VERSION}")
endif ()

if (DPDK_FOUND AND NOT TARGET DPDK::driver_sdk)
  find_path(DPDK_DRIVER_SDK_INCLUDE_DIR bus_vdev_driver.h
          HINTS ${PC_DPDK_INCLUDE_DIRS})

  if (DPDK_DRIVER_SDK_INCLUDE_DIR)
    add_library(DPDK::driver_sdk INTERFACE IMPORTED GLOBAL)

    target_link_libraries(DPDK::driver_sdk INTERFACE DPDK::dpdk)
    target_include_directories(DPDK::driver_sdk
            INTERFACE ${DPDK_DRIVER_SDK_INCLUDE_DIR})
    target_compile_definitions(DPDK::driver_sdk INTERFACE ALLOW_INTERNAL_API)

    message(STATUS "DPDK driver SDK found at ${DPDK_DRIVER_SDK_INCLUDE_DIR}")
  else ()
    message(STATUS "DPDK driver SDK not found — PMD targets will be skipped. "
            "Install DPDK with -Denable_driver_sdk=true to enable.")
  endif ()
endif ()
