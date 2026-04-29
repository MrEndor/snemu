#pragma once
#include <stdbool.h>
#include <stdint.h>

#define SNEMU_RSS_KEY_SIZE  40
#define SNEMU_RSS_RETA_SIZE 128

struct rte_eth_rss_conf;
struct rte_eth_rss_reta_entry64;
struct rte_mbuf;
struct snemu_port;

struct snemu_rss_ctx {
  bool enabled;
  uint64_t hash_types;
  uint8_t key[SNEMU_RSS_KEY_SIZE];
  uint8_t key_be[SNEMU_RSS_KEY_SIZE];
  uint16_t reta[SNEMU_RSS_RETA_SIZE];
};

int feat_rss_configure(struct snemu_port* port,
                       const struct rte_eth_rss_conf* cfg,
                       uint16_t nb_rx_queues);

uint32_t feat_rss_hash_ipv4(uint32_t sip,
                            uint32_t dip,
                            const uint8_t key_be[SNEMU_RSS_KEY_SIZE]);

uint32_t feat_rss_hash_ipv4_l4(uint32_t sip,
                               uint32_t dip,
                               uint16_t sport,
                               uint16_t dport,
                               const uint8_t key_be[SNEMU_RSS_KEY_SIZE]);

void feat_rss_classify(struct snemu_port* port, struct rte_mbuf* pkt);

int feat_rss_reta_update(struct snemu_port* port,
                         const struct rte_eth_rss_reta_entry64* conf,
                         uint16_t reta_size);

int feat_rss_reta_query(const struct snemu_port* port,
                        struct rte_eth_rss_reta_entry64* conf,
                        uint16_t reta_size);

int feat_rss_hash_update(struct snemu_port* port,
                         const struct rte_eth_rss_conf* cfg);

int feat_rss_hash_conf_get(const struct snemu_port* port,
                           struct rte_eth_rss_conf* cfg);
