#include <rte_ether.h>
#include <rte_mbuf.h>
#include <snemu/core/emulator_node.h>
#include <snemu/features/mac_filter.h>
#include <snemu/features/rss_toeplitz.h>
#include <snemu/port.h>
#include <stddef.h>

uint16_t emulator_node_process(struct snemu_port* port,
                               struct rte_mbuf** pkts,
                               uint16_t nb) {
  if (port == nullptr) {
    return nb;
  }

  // Step 1: MAC filter — drop mismatched dst MAC, compact in place,
  // free dropped mbufs.
  uint16_t kept = 0;
  for (uint16_t i = 0; i < nb; i++) {
    struct rte_mbuf* mbuf = pkts[i];
    if (mbuf == nullptr) {
      continue;
    }
    if (rte_pktmbuf_pkt_len(mbuf) < sizeof(struct rte_ether_hdr)) {
      rte_pktmbuf_free(mbuf);
      continue;
    }
    const struct rte_ether_hdr* eth =
      rte_pktmbuf_mtod(mbuf, const struct rte_ether_hdr*);
    if (!feat_mac_filter_match(port, &eth->dst_addr)) {
      rte_pktmbuf_free(mbuf);
      continue;
    }
    pkts[kept++] = mbuf;
  }

  // Step 2: RSS — classify survivors only when configured.
  if (port->rss.enabled) {
    for (uint16_t i = 0; i < kept; i++) {
      feat_rss_classify(port, pkts[i]);
    }
  }
  return kept;
}
