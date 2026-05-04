#include <errno.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>

extern "C" {
#include <rte_ether.h>
#include <snemu/features/mac_filter.h>
#include <snemu/port.h>
}

namespace {

constexpr rte_ether_addr kDefault{
  .addr_bytes = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}};
constexpr rte_ether_addr kOther{
  .addr_bytes = {0x02, 0x00, 0x00, 0x00, 0x00, 0x99}};
constexpr rte_ether_addr kAdded{
  .addr_bytes = {0x02, 0x00, 0x00, 0x00, 0xab, 0xcd}};
constexpr rte_ether_addr kBroadcast{
  .addr_bytes = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
constexpr rte_ether_addr kMulticast{
  .addr_bytes = {0x01, 0x00, 0x5e, 0x00, 0x00, 0xfb}};
constexpr rte_ether_addr kMulticast2{
  .addr_bytes = {0x01, 0x00, 0x5e, 0x12, 0x34, 0x56}};

snemu_port make_port() {
  snemu_port port{};
  EXPECT_EQ(0, feat_mac_filter_init(&port, &kDefault));
  return port;
}

} // namespace

TEST(MacFilter, InitSeatsDefaultAtSlotZero) {
  snemu_port port = make_port();
  EXPECT_TRUE(port.mac_filter.used[0]);
  EXPECT_FALSE(port.mac_filter.promisc);
  EXPECT_FALSE(port.mac_filter.allmulticast);
  EXPECT_EQ(0u, port.mac_filter.mc_count);
  EXPECT_TRUE(feat_mac_filter_match(&port, &kDefault));
}

TEST(MacFilter, RejectsNullArgs) {
  snemu_port port{};
  EXPECT_EQ(-EINVAL, feat_mac_filter_init(&port, nullptr));
  EXPECT_EQ(-EINVAL, feat_mac_filter_init(nullptr, &kDefault));
  EXPECT_EQ(-EINVAL, feat_mac_filter_add(nullptr, &kAdded, 1));
}

TEST(MacFilter, UnknownUnicastDropped) {
  snemu_port port = make_port();
  EXPECT_FALSE(feat_mac_filter_match(&port, &kOther));
}

TEST(MacFilter, AddRemoveUnicast) {
  snemu_port port = make_port();
  EXPECT_FALSE(feat_mac_filter_match(&port, &kAdded));

  ASSERT_EQ(0, feat_mac_filter_add(&port, &kAdded, 5));
  EXPECT_TRUE(feat_mac_filter_match(&port, &kAdded));

  feat_mac_filter_remove(&port, 5);
  EXPECT_FALSE(feat_mac_filter_match(&port, &kAdded));
}

TEST(MacFilter, RemoveSlotZeroIsNoop) {
  snemu_port port = make_port();
  feat_mac_filter_remove(&port, 0);
  EXPECT_TRUE(feat_mac_filter_match(&port, &kDefault));
}

TEST(MacFilter, AddRejectsOutOfRange) {
  snemu_port port = make_port();
  EXPECT_EQ(-EINVAL, feat_mac_filter_add(&port, &kAdded, SNEMU_MAX_UC_MACS));
}

TEST(MacFilter, SetDefaultReplacesSlotZero) {
  snemu_port port = make_port();
  ASSERT_EQ(0, feat_mac_filter_set_default(&port, &kOther));
  EXPECT_TRUE(feat_mac_filter_match(&port, &kOther));
  EXPECT_FALSE(feat_mac_filter_match(&port, &kDefault));
}

TEST(MacFilter, PromiscPassesAllUnicast) {
  snemu_port port = make_port();
  feat_mac_filter_set_promisc(&port, true);
  EXPECT_TRUE(feat_mac_filter_match(&port, &kOther));
  EXPECT_TRUE(feat_mac_filter_match(&port, &kAdded));

  feat_mac_filter_set_promisc(&port, false);
  EXPECT_FALSE(feat_mac_filter_match(&port, &kOther));
}

TEST(MacFilter, BroadcastAlwaysPasses) {
  snemu_port port = make_port();
  EXPECT_TRUE(feat_mac_filter_match(&port, &kBroadcast));
}

TEST(MacFilter, MulticastDroppedByDefault) {
  snemu_port port = make_port();
  EXPECT_FALSE(feat_mac_filter_match(&port, &kMulticast));
}

TEST(MacFilter, MulticastInListPasses) {
  snemu_port port = make_port();
  std::array<rte_ether_addr, 1> list{kMulticast};
  ASSERT_EQ(0, feat_mac_filter_set_mc_list(&port, list.data(), list.size()));
  EXPECT_TRUE(feat_mac_filter_match(&port, &kMulticast));
  EXPECT_FALSE(feat_mac_filter_match(&port, &kMulticast2));
}

TEST(MacFilter, AllmulticastPassesAnyMulticast) {
  snemu_port port = make_port();
  feat_mac_filter_set_allmulticast(&port, true);
  EXPECT_TRUE(feat_mac_filter_match(&port, &kMulticast));
  EXPECT_TRUE(feat_mac_filter_match(&port, &kMulticast2));
  EXPECT_FALSE(feat_mac_filter_match(&port, &kOther));

  feat_mac_filter_set_allmulticast(&port, false);
  EXPECT_FALSE(feat_mac_filter_match(&port, &kMulticast));
}

TEST(MacFilter, McListRejectsOversize) {
  snemu_port port = make_port();
  std::array<rte_ether_addr, 1> list{kMulticast};
  EXPECT_EQ(
    -EINVAL,
    feat_mac_filter_set_mc_list(&port, list.data(), SNEMU_MAX_MC_MACS + 1));
}

TEST(MacFilter, McListEmptyClears) {
  snemu_port port = make_port();
  std::array<rte_ether_addr, 1> list{kMulticast};
  ASSERT_EQ(0, feat_mac_filter_set_mc_list(&port, list.data(), list.size()));
  ASSERT_TRUE(feat_mac_filter_match(&port, &kMulticast));

  ASSERT_EQ(0, feat_mac_filter_set_mc_list(&port, nullptr, 0));
  EXPECT_EQ(0u, port.mac_filter.mc_count);
  EXPECT_FALSE(feat_mac_filter_match(&port, &kMulticast));
}

TEST(MacFilter, MatchHandlesNull) {
  snemu_port port = make_port();
  EXPECT_FALSE(feat_mac_filter_match(nullptr, &kDefault));
  EXPECT_FALSE(feat_mac_filter_match(&port, nullptr));
}
