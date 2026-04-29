add_library(snemu_features STATIC
        io/capture_null.c
        core/frontend_io.c
        core/emulator_node.c
        features/mac_filter.c
        features/rss_toeplitz.c)

target_include_directories(snemu_features
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(snemu_features
        PUBLIC  DPDK::dpdk
        PRIVATE snemu_compile_opts snemu_sanitizers)

if (TARGET DPDK::driver_sdk)
    add_library(snemu_pmd STATIC
            pmd/ethdev_pnp.c
            pmd/ethdev_ops.c)

    target_include_directories(snemu_pmd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(snemu_pmd
            PUBLIC  snemu_features DPDK::driver_sdk
            PRIVATE snemu_compile_opts snemu_sanitizers)
endif ()
