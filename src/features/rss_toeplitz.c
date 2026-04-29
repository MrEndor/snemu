#include <errno.h>
#include <limits.h>
#include <rte_byteorder.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_mbuf_core.h>
#include <rte_tcp.h>
#include <rte_thash.h>
#include <rte_udp.h>
#include <snemu/features/rss_toeplitz.h>
#include <snemu/port.h>
#include <stddef.h>
#include <string.h>

static const uint8_t DEFAULT_RSS_KEY[SNEMU_RSS_KEY_SIZE] = {
  0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2, 0x41, 0x67,
  0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0, 0xd0, 0xca, 0x2b, 0xcb,
  0xae, 0x7b, 0x30, 0xb4, 0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30,
  0xf2, 0x0c, 0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa,
};

static void convert_key(struct snemu_rss_ctx* ctx) {
  rte_convert_rss_key((const uint32_t*)(const void*)ctx->key,
                      (uint32_t*)(void*)ctx->key_be,
                      SNEMU_RSS_KEY_SIZE);
}

int feat_rss_configure(struct snemu_port* port,
                       const struct rte_eth_rss_conf* cfg,
                       uint16_t nb_rx_queues) {
  if (port == nullptr || nb_rx_queues == 0) {
    return -EINVAL;
  }
  struct snemu_rss_ctx* ctx = &port->rss;

  if (cfg != nullptr && cfg->rss_key != nullptr &&
      cfg->rss_key_len == SNEMU_RSS_KEY_SIZE) {
    memcpy(ctx->key, cfg->rss_key, SNEMU_RSS_KEY_SIZE);
  } else {
    memcpy(ctx->key, DEFAULT_RSS_KEY, SNEMU_RSS_KEY_SIZE);
  }
  convert_key(ctx);

  ctx->hash_types = (cfg != nullptr) ? cfg->rss_hf : 0;

  for (uint16_t i = 0; i < SNEMU_RSS_RETA_SIZE; i++) {
    ctx->reta[i] = (uint16_t)(i % nb_rx_queues);
  }

  ctx->enabled = true;
  return 0;
}

uint32_t feat_rss_hash_ipv4(uint32_t sip,
                            uint32_t dip,
                            const uint8_t key_be[SNEMU_RSS_KEY_SIZE]) {
  uint32_t buf[2] = {rte_cpu_to_be_32(sip), rte_cpu_to_be_32(dip)};
  return rte_softrss_be(buf, 2, key_be);
}

uint32_t feat_rss_hash_ipv4_l4(uint32_t sip,
                               uint32_t dip,
                               uint16_t sport,
                               uint16_t dport,
                               const uint8_t key_be[SNEMU_RSS_KEY_SIZE]) {
  uint32_t ports = ((uint32_t)sport << (CHAR_BIT * sizeof(uint16_t))) | dport;
  uint32_t buf[3] = {
    rte_cpu_to_be_32(sip),
    rte_cpu_to_be_32(dip),
    rte_cpu_to_be_32(ports),
  };
  return rte_softrss_be(buf, sizeof(buf) / sizeof(buf[0]), key_be);
}

void feat_rss_classify(struct snemu_port* port, struct rte_mbuf* pkt) {
  if (port == nullptr || pkt == nullptr || !port->rss.enabled) {
    return;
  }

  if (rte_pktmbuf_pkt_len(pkt) <
      sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)) {
    return;
  }

  const struct rte_ether_hdr* eth =
    rte_pktmbuf_mtod(pkt, const struct rte_ether_hdr*);
  if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
    return;
  }

  const struct rte_ipv4_hdr* ip =
    rte_pktmbuf_mtod_offset(pkt, const struct rte_ipv4_hdr*, sizeof(*eth));

  uint32_t sip = rte_be_to_cpu_32(ip->src_addr);
  uint32_t dip = rte_be_to_cpu_32(ip->dst_addr);

  bool want_tcp = (port->rss.hash_types & RTE_ETH_RSS_NONFRAG_IPV4_TCP) != 0;
  bool want_udp = (port->rss.hash_types & RTE_ETH_RSS_NONFRAG_IPV4_UDP) != 0;
  uint32_t hash = 0;

  size_t l4_off = sizeof(*eth) + rte_ipv4_hdr_len(ip);

  if (ip->next_proto_id == IPPROTO_TCP && want_tcp &&
      rte_pktmbuf_pkt_len(pkt) >= l4_off + sizeof(struct rte_tcp_hdr)) {
    const struct rte_tcp_hdr* tcp =
      rte_pktmbuf_mtod_offset(pkt, const struct rte_tcp_hdr*, l4_off);
    hash = feat_rss_hash_ipv4_l4(sip,
                                 dip,
                                 rte_be_to_cpu_16(tcp->src_port),
                                 rte_be_to_cpu_16(tcp->dst_port),
                                 port->rss.key_be);
  } else if (ip->next_proto_id == IPPROTO_UDP && want_udp &&
             rte_pktmbuf_pkt_len(pkt) >= l4_off + sizeof(struct rte_udp_hdr)) {
    const struct rte_udp_hdr* udp =
      rte_pktmbuf_mtod_offset(pkt, const struct rte_udp_hdr*, l4_off);
    hash = feat_rss_hash_ipv4_l4(sip,
                                 dip,
                                 rte_be_to_cpu_16(udp->src_port),
                                 rte_be_to_cpu_16(udp->dst_port),
                                 port->rss.key_be);
  } else if ((port->rss.hash_types & RTE_ETH_RSS_IPV4) != 0) {
    hash = feat_rss_hash_ipv4(sip, dip, port->rss.key_be);
  } else {
    return;
  }

  pkt->hash.rss = hash;
  pkt->ol_flags |= RTE_MBUF_F_RX_RSS_HASH;
}

int feat_rss_reta_update(struct snemu_port* port,
                         const struct rte_eth_rss_reta_entry64* conf,
                         uint16_t reta_size) {
  if (port == nullptr || conf == nullptr || reta_size != SNEMU_RSS_RETA_SIZE) {
    return -EINVAL;
  }

  uint16_t groups = reta_size / RTE_ETH_RETA_GROUP_SIZE;
  for (uint16_t g = 0; g < groups; g++) {
    for (uint16_t b = 0; b < RTE_ETH_RETA_GROUP_SIZE; b++) {
      if ((conf[g].mask & ((uint64_t)1 << b)) != 0) {
        port->rss.reta[(g * RTE_ETH_RETA_GROUP_SIZE) + b] = conf[g].reta[b];
      }
    }
  }
  return 0;
}

int feat_rss_reta_query(const struct snemu_port* port,
                        struct rte_eth_rss_reta_entry64* conf,
                        uint16_t reta_size) {
  if (port == nullptr || conf == nullptr || reta_size != SNEMU_RSS_RETA_SIZE) {
    return -EINVAL;
  }

  uint16_t groups = reta_size / RTE_ETH_RETA_GROUP_SIZE;
  for (uint16_t g = 0; g < groups; g++) {
    for (uint16_t b = 0; b < RTE_ETH_RETA_GROUP_SIZE; b++) {
      if ((conf[g].mask & ((uint64_t)1 << b)) != 0) {
        conf[g].reta[b] = port->rss.reta[(g * RTE_ETH_RETA_GROUP_SIZE) + b];
      }
    }
  }
  return 0;
}

int feat_rss_hash_update(struct snemu_port* port,
                         const struct rte_eth_rss_conf* cfg) {
  if (port == nullptr || cfg == nullptr) {
    return -EINVAL;
  }
  if (cfg->rss_key != nullptr) {
    if (cfg->rss_key_len != SNEMU_RSS_KEY_SIZE) {
      return -EINVAL;
    }
    memcpy(port->rss.key, cfg->rss_key, SNEMU_RSS_KEY_SIZE);
    convert_key(&port->rss);
  }
  port->rss.hash_types = cfg->rss_hf;
  return 0;
}

int feat_rss_hash_conf_get(const struct snemu_port* port,
                           struct rte_eth_rss_conf* cfg) {
  if (port == nullptr || cfg == nullptr) {
    return -EINVAL;
  }
  cfg->rss_hf = port->rss.hash_types;
  if (cfg->rss_key != nullptr) {
    if (cfg->rss_key_len < SNEMU_RSS_KEY_SIZE) {
      return -EINVAL;
    }
    memcpy(cfg->rss_key, port->rss.key, SNEMU_RSS_KEY_SIZE);
  }
  cfg->rss_key_len = SNEMU_RSS_KEY_SIZE;
  return 0;
}
