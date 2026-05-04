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
        PUBLIC DPDK::dpdk
        PRIVATE snemu_compile_opts snemu_sanitizers)

add_library(snemu_pmd SHARED
        pmd/ethdev_pnp.c
        pmd/ethdev_ops.c
        $<TARGET_OBJECTS:snemu_features>)

set_target_properties(snemu_pmd PROPERTIES
        OUTPUT_NAME rte_net_snemu
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_include_directories(snemu_pmd PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        $<TARGET_PROPERTY:DPDK::driver_sdk,INTERFACE_INCLUDE_DIRECTORIES>)

target_compile_definitions(snemu_pmd PRIVATE
        $<TARGET_PROPERTY:DPDK::driver_sdk,INTERFACE_COMPILE_DEFINITIONS>)

target_compile_options(snemu_pmd PRIVATE
        $<TARGET_PROPERTY:DPDK::driver_sdk,INTERFACE_COMPILE_OPTIONS>)

target_link_libraries(snemu_pmd
        PRIVATE snemu_compile_opts snemu_sanitizers)

target_link_options(snemu_pmd PRIVATE
        "LINKER:--unresolved-symbols=ignore-all")
