#pragma once
#include <snemu/features/mac_filter.h>
#include <snemu/features/rss_toeplitz.h>
#include <stdint.h>

struct rte_eth_dev;
struct snemu_backend;

#define SNEMU_MAX_QUEUES  16
#define SNEMU_MAX_RX_DESC 4096
#define SNEMU_MAX_TX_DESC 4096

struct snemu_port;

struct snemu_rx_queue {
  struct snemu_port* port;
  uint16_t queue_id;
  uint64_t pkts;
};

struct snemu_tx_queue {
  struct snemu_port* port;
  uint16_t queue_id;
  uint64_t pkts;
};

struct snemu_port {
  struct rte_eth_dev* eth_dev;
  struct snemu_backend* backend;

  uint16_t nb_rx_queues;
  uint16_t nb_tx_queues;

  struct snemu_rx_queue* rx_queues[SNEMU_MAX_QUEUES];
  struct snemu_tx_queue* tx_queues[SNEMU_MAX_QUEUES];

  struct snemu_mac_filter_ctx mac_filter;
  struct snemu_rss_ctx rss;
};
