#pragma once
#include <rte_ethdev.h>

struct rte_mbuf;

extern const struct eth_dev_ops PMD_ETH_OPS;

uint16_t pmd_rx_pkt_burst(void* rx_queue, struct rte_mbuf** pkts, uint16_t nb);
uint16_t pmd_tx_pkt_burst(void* tx_queue, struct rte_mbuf** pkts, uint16_t nb);
