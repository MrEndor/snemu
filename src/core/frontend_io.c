#include <snemu/core/frontend_io.h>

uint16_t frontend_io_pull(struct snemu_backend* backend,
                          struct rte_mbuf** pkts,
                          uint16_t nb) {
  if (backend == nullptr || backend->ops == nullptr ||
      backend->ops->rx_burst == nullptr) {
    return 0;
  }
  return backend->ops->rx_burst(backend->state, pkts, nb);
}

uint16_t frontend_io_push(struct snemu_backend* backend,
                          struct rte_mbuf** pkts,
                          uint16_t nb) {
  if (backend == nullptr || backend->ops == nullptr ||
      backend->ops->tx_burst == nullptr) {
    return 0;
  }
  return backend->ops->tx_burst(backend->state, pkts, nb);
}
