# Builds the DPDK submodule at third_party/dpdk with -Denable_driver_sdk=true
# so out-of-tree PMDs can include bus_vdev_driver.h / ethdev_driver.h.
#
# Runs at configure time via execute_process. A pure ExternalProject_Add
# approach would defer the install to build phase, but then
# pkg_check_modules(libdpdk) in FindDPDK.cmake would have nothing to find on
# the first configure — forcing either manual IMPORTED-target plumbing for
# ~100 DPDK libraries or a two-phase configure UX. Kept inline; idempotent
# via the sentinel header below.
#
# Activated by include(BootstrapDpdk) from the top-level CMakeLists.txt,
# gated by -DSNEMU_BOOTSTRAP_DPDK=ON.

include_guard(GLOBAL)

set(SNEMU_DPDK_SOURCE_DIR
    "${PROJECT_SOURCE_DIR}/third_party/dpdk"
    CACHE PATH "DPDK source tree (git submodule)")

set(SNEMU_DPDK_INSTALL_DIR
    "${PROJECT_SOURCE_DIR}/dpdk-install"
    CACHE PATH "Install prefix for the bootstrapped DPDK")

set(SNEMU_DPDK_BUILDTYPE "debugoptimized"
    CACHE STRING "Meson buildtype for the bootstrapped DPDK")

set(_dpdk_build_dir "${SNEMU_DPDK_INSTALL_DIR}/_build")
set(_dpdk_sentinel  "${SNEMU_DPDK_INSTALL_DIR}/include/bus_vdev_driver.h")

if (NOT EXISTS "${SNEMU_DPDK_SOURCE_DIR}/meson.build")
  message(FATAL_ERROR
    "DPDK submodule not initialised at ${SNEMU_DPDK_SOURCE_DIR}. "
    "Run: git submodule update --init third_party/dpdk")
endif ()

if (EXISTS "${_dpdk_sentinel}")
  message(STATUS "DPDK bootstrap: reusing ${SNEMU_DPDK_INSTALL_DIR}")
else ()
  find_program(MESON_EXE meson REQUIRED)
  find_program(NINJA_EXE ninja REQUIRED)

  if (NOT EXISTS "${_dpdk_build_dir}/build.ninja")
    message(STATUS "DPDK bootstrap: meson setup (${SNEMU_DPDK_BUILDTYPE})")
    execute_process(
      COMMAND ${MESON_EXE} setup
              --prefix=${SNEMU_DPDK_INSTALL_DIR}
              --buildtype=${SNEMU_DPDK_BUILDTYPE}
              -Denable_driver_sdk=true
              -Dtests=false
              -Dexamples=
              -Ddisable_drivers=*/*
              "${_dpdk_build_dir}" "${SNEMU_DPDK_SOURCE_DIR}"
      COMMAND_ERROR_IS_FATAL ANY)
  endif ()

  message(STATUS
    "DPDK bootstrap: meson install (first run takes several minutes)")
  execute_process(
    COMMAND ${MESON_EXE} install -C "${_dpdk_build_dir}"
    COMMAND_ERROR_IS_FATAL ANY)
endif ()

# Make pkg_check_modules pick up libdpdk.pc from our install prefix via
# CMAKE_PREFIX_PATH (see PKG_CONFIG_USE_CMAKE_PREFIX_PATH in FindPkgConfig).
# No ENV{} mutation, no file(GLOB): deterministic, survives re-configure.
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
list(PREPEND CMAKE_PREFIX_PATH "${SNEMU_DPDK_INSTALL_DIR}")
