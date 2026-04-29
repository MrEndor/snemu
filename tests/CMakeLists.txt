include(SnemuTesting)

snemu_add_gtest(test_rss
        SOURCES test_rss.cc
        LIBRARIES snemu_features DPDK::dpdk)

snemu_add_gtest(test_mac_filter
        SOURCES test_mac_filter.cc
        LIBRARIES snemu_features DPDK::dpdk)
