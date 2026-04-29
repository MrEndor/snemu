#include <rte_mbuf.h>
#include <snemu/core/frontend_io.h>

static uint16_t null_rx(void* state, struct rte_mbuf** pkts, uint16_t nb) {
  (void)state;
  (void)pkts;
  (void)nb;
  return 0;
}

static uint16_t null_tx(void* state, struct rte_mbuf** pkts, uint16_t nb) {
  (void)state;
  for (uint16_t i = 0; i < nb; i++) {
    rte_pktmbuf_free(pkts[i]);
  }
  return nb;
}

static const struct snemu_backend_ops NULL_OPS = {
  .rx_burst = null_rx,
  .tx_burst = null_tx,
};

static struct snemu_backend null_instance = {
  .ops = &NULL_OPS,
  .state = nullptr,
};

struct snemu_backend* null_backend(void) {
  return &null_instance;
}
