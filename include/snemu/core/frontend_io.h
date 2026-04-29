#pragma once
#include <stdint.h>

struct rte_mbuf;

struct snemu_backend_ops {
  uint16_t (*rx_burst)(void* state, struct rte_mbuf** pkts, uint16_t nb);
  uint16_t (*tx_burst)(void* state, struct rte_mbuf** pkts, uint16_t nb);
};

struct snemu_backend {
  const struct snemu_backend_ops* ops;
  void* state;
};

uint16_t frontend_io_pull(struct snemu_backend* backend,
                          struct rte_mbuf** pkts,
                          uint16_t nb);

uint16_t frontend_io_push(struct snemu_backend* backend,
                          struct rte_mbuf** pkts,
                          uint16_t nb);

struct snemu_backend* null_backend(void);
