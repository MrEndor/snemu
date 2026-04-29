#include <endian.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>

extern "C" {
#include <rte_byteorder.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_thash.h>
#include <snemu/features/rss_toeplitz.h>
#include <snemu/port.h>
}

namespace {

constexpr std::array<uint8_t, SNEMU_RSS_KEY_SIZE> kKey = {
  0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2, 0x41, 0x67,
  0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0, 0xd0, 0xca, 0x2b, 0xcb,
  0xae, 0x7b, 0x30, 0xb4, 0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30,
  0xf2, 0x0c, 0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa,
};

struct Tuple {
  uint32_t sip;
  uint32_t dip;
  uint16_t sp;
  uint16_t dp;
};

constexpr std::array<Tuple, 3> kVecs = {{
  {RTE_IPV4(192, 168, 0, 1), RTE_IPV4(10, 0, 0, 1), 1024, 80},
  {RTE_IPV4(172, 16, 5, 7), RTE_IPV4(8, 8, 8, 8), 5000, 53},
  {RTE_IPV4(10, 1, 2, 3), RTE_IPV4(10, 1, 2, 4), 443, 34567},
}};

snemu_port make_port(uint64_t hf = RTE_ETH_RSS_NONFRAG_IPV4_TCP) {
  snemu_port port{};
  rte_eth_rss_conf conf{};
  std::array<uint8_t, SNEMU_RSS_KEY_SIZE> mutable_key = kKey;
  conf.rss_key = mutable_key.data();
  conf.rss_key_len = SNEMU_RSS_KEY_SIZE;
  conf.rss_hf = hf;
  EXPECT_EQ(0, feat_rss_configure(&port, &conf, 4));
  return port;
}

} // namespace

TEST(RssToeplitz, MatchesSoftrssBe) {
  snemu_port port = make_port();

  for (const auto& v : kVecs) {
    uint32_t buf[3] = {
      rte_cpu_to_be_32(v.sip),
      rte_cpu_to_be_32(v.dip),
      rte_cpu_to_be_32((static_cast<uint32_t>(v.sp) << 16) | v.dp),
    };
    uint32_t expected = rte_softrss_be(buf, 3, port.rss.key_be);
    uint32_t got =
      feat_rss_hash_ipv4_l4(v.sip, v.dip, v.sp, v.dp, port.rss.key_be);
    EXPECT_EQ(expected, got)
      << "tuple sip=" << std::hex << v.sip << " dip=" << v.dip;
  }
}

TEST(RssToeplitz, Ipv4OnlyMatchesReference) {
  snemu_port port = make_port(RTE_ETH_RSS_IPV4);

  uint32_t buf[2] = {
    rte_cpu_to_be_32(kVecs[0].sip),
    rte_cpu_to_be_32(kVecs[0].dip),
  };
  uint32_t expected = rte_softrss_be(buf, 2, port.rss.key_be);
  uint32_t got =
    feat_rss_hash_ipv4(kVecs[0].sip, kVecs[0].dip, port.rss.key_be);
  EXPECT_EQ(expected, got);
}

TEST(RssReta, DefaultRoundRobin) {
  snemu_port port = make_port();
  EXPECT_TRUE(port.rss.enabled);

  for (uint16_t i = 0; i < SNEMU_RSS_RETA_SIZE; i++) {
    EXPECT_EQ(i % 4, port.rss.reta[i]);
  }
}

TEST(RssReta, UpdateQueryRoundTrip) {
  snemu_port port = make_port();

  constexpr uint16_t kGroups = SNEMU_RSS_RETA_SIZE / RTE_ETH_RETA_GROUP_SIZE;
  std::array<rte_eth_rss_reta_entry64, kGroups> conf{};
  conf[0].mask = (1ULL << 5) | (1ULL << 7);
  conf[0].reta[5] = 3;
  conf[0].reta[7] = 2;

  ASSERT_EQ(0, feat_rss_reta_update(&port, conf.data(), SNEMU_RSS_RETA_SIZE));
  EXPECT_EQ(3, port.rss.reta[5]);
  EXPECT_EQ(2, port.rss.reta[7]);
  EXPECT_EQ(1 % 4, port.rss.reta[1]);

  std::array<rte_eth_rss_reta_entry64, kGroups> readback{};
  readback[0].mask = (1ULL << 5) | (1ULL << 7);
  ASSERT_EQ(0,
            feat_rss_reta_query(&port, readback.data(), SNEMU_RSS_RETA_SIZE));
  EXPECT_EQ(3, readback[0].reta[5]);
  EXPECT_EQ(2, readback[0].reta[7]);
}

TEST(RssReta, RejectsWrongSize) {
  snemu_port port = make_port();
  rte_eth_rss_reta_entry64 conf{};
  EXPECT_EQ(-EINVAL, feat_rss_reta_update(&port, &conf, 64));
  EXPECT_EQ(-EINVAL, feat_rss_reta_query(&port, &conf, 64));
}

TEST(RssHashConf, UpdateQueryRoundTrip) {
  snemu_port port = make_port();

  std::array<uint8_t, SNEMU_RSS_KEY_SIZE> new_key{};
  for (size_t i = 0; i < new_key.size(); i++) {
    new_key[i] = static_cast<uint8_t>(i * 3 + 1);
  }
  rte_eth_rss_conf update{};
  update.rss_key = new_key.data();
  update.rss_key_len = SNEMU_RSS_KEY_SIZE;
  update.rss_hf = RTE_ETH_RSS_IPV4;

  ASSERT_EQ(0, feat_rss_hash_update(&port, &update));
  EXPECT_EQ(RTE_ETH_RSS_IPV4, port.rss.hash_types);
  EXPECT_EQ(0, std::memcmp(port.rss.key, new_key.data(), SNEMU_RSS_KEY_SIZE));

  std::array<uint8_t, SNEMU_RSS_KEY_SIZE> out_key{};
  rte_eth_rss_conf query{};
  query.rss_key = out_key.data();
  query.rss_key_len = SNEMU_RSS_KEY_SIZE;

  ASSERT_EQ(0, feat_rss_hash_conf_get(&port, &query));
  EXPECT_EQ(RTE_ETH_RSS_IPV4, query.rss_hf);
  EXPECT_EQ(SNEMU_RSS_KEY_SIZE, query.rss_key_len);
  EXPECT_EQ(0, std::memcmp(out_key.data(), new_key.data(), SNEMU_RSS_KEY_SIZE));
}

TEST(RssHashConf, RejectsWrongKeyLength) {
  snemu_port port = make_port();

  std::array<uint8_t, 32> short_key{};
  rte_eth_rss_conf update{};
  update.rss_key = short_key.data();
  update.rss_key_len = short_key.size();
  update.rss_hf = 0;

  EXPECT_EQ(-EINVAL, feat_rss_hash_update(&port, &update));
}
