#include <bus_vdev_driver.h>
#include <errno.h>
#include <ethdev_driver.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <stdint.h>

#include "ethdev_pnp.h"
#include "snemu/core/emulator_node.h"
#include "snemu/core/frontend_io.h"
#include "snemu/features/mac_filter.h"
#include "snemu/features/rss_toeplitz.h"
#include "snemu/port.h"

static int dev_configure(struct rte_eth_dev* dev) {
  struct snemu_port* port = dev->data->dev_private;
  port->nb_rx_queues = dev->data->nb_rx_queues;
  port->nb_tx_queues = dev->data->nb_tx_queues;

  if ((dev->data->dev_conf.rxmode.mq_mode & RTE_ETH_MQ_RX_RSS_FLAG) != 0) {
    return feat_rss_configure(
      port, &dev->data->dev_conf.rx_adv_conf.rss_conf, dev->data->nb_rx_queues);
  }
  port->rss.enabled = false;
  return 0;
}

static int dev_start(struct rte_eth_dev* dev) {
  dev->data->dev_link.link_status = RTE_ETH_LINK_UP;
  return 0;
}

static int dev_stop(struct rte_eth_dev* dev) {
  dev->data->dev_link.link_status = RTE_ETH_LINK_DOWN;
  return 0;
}

static int dev_close(struct rte_eth_dev* dev) {
  return 0;
}

static int dev_infos_get(struct rte_eth_dev* /*unsued*/,
                         struct rte_eth_dev_info* info) {
  info->max_rx_queues = SNEMU_MAX_QUEUES;
  info->max_tx_queues = SNEMU_MAX_QUEUES;
  info->min_rx_bufsize = RTE_ETHER_MIN_LEN;
  info->max_rx_pktlen = RTE_ETHER_MAX_JUMBO_FRAME_LEN;
  info->max_mac_addrs = SNEMU_MAX_UC_MACS;
  info->rx_desc_lim = (struct rte_eth_desc_lim){
    .nb_min = 1, .nb_max = SNEMU_MAX_RX_DESC, .nb_align = 1};
  info->tx_desc_lim = (struct rte_eth_desc_lim){
    .nb_min = 1, .nb_max = SNEMU_MAX_TX_DESC, .nb_align = 1};
  info->flow_type_rss_offloads = RTE_ETH_RSS_IPV4 |
                                 RTE_ETH_RSS_NONFRAG_IPV4_TCP |
                                 RTE_ETH_RSS_NONFRAG_IPV4_UDP;
  info->reta_size = SNEMU_RSS_RETA_SIZE;
  info->hash_key_size = SNEMU_RSS_KEY_SIZE;
  return 0;
}

static int rx_queue_setup(struct rte_eth_dev* dev,
                          uint16_t qid,
                          uint16_t nb_desc,
                          unsigned int socket_id,
                          const struct rte_eth_rxconf* conf,
                          struct rte_mempool* mp) {
  (void)nb_desc;
  (void)conf;
  (void)mp;
  struct snemu_port* port = dev->data->dev_private;
  struct snemu_rx_queue* rxq =
    rte_zmalloc_socket("snemu_rxq", sizeof(*rxq), 0, (int)socket_id);
  if (rxq == nullptr) {
    return -ENOMEM;
  }
  rxq->port = port;
  rxq->queue_id = qid;
  dev->data->rx_queues[qid] = rxq;
  if (qid < SNEMU_MAX_QUEUES) {
    port->rx_queues[qid] = rxq;
  }
  return 0;
}

static int tx_queue_setup(struct rte_eth_dev* dev,
                          uint16_t qid,
                          uint16_t nb_desc,
                          unsigned int socket_id,
                          const struct rte_eth_txconf* conf) {
  struct snemu_port* port = dev->data->dev_private;
  struct snemu_tx_queue* txq =
    rte_zmalloc_socket("snemu_txq", sizeof(*txq), 0, (int)socket_id);
  if (txq == nullptr) {
    return -ENOMEM;
  }
  txq->port = port;
  txq->queue_id = qid;
  dev->data->tx_queues[qid] = txq;
  if (qid < SNEMU_MAX_QUEUES) {
    port->tx_queues[qid] = txq;
  }
  return 0;
}

static void rx_queue_release(struct rte_eth_dev* dev, uint16_t qid) {
  struct snemu_port* port = dev->data->dev_private;
  if (qid < SNEMU_MAX_QUEUES) {
    port->rx_queues[qid] = nullptr;
  }
  rte_free(dev->data->rx_queues[qid]);
  dev->data->rx_queues[qid] = nullptr;
}

static void tx_queue_release(struct rte_eth_dev* dev, uint16_t qid) {
  struct snemu_port* port = dev->data->dev_private;
  if (qid < SNEMU_MAX_QUEUES) {
    port->tx_queues[qid] = nullptr;
  }
  rte_free(dev->data->tx_queues[qid]);
  dev->data->tx_queues[qid] = nullptr;
}

static int link_update(struct rte_eth_dev* dev, int wait_to_complete) {
  return 0;
}

static int rss_hash_update(struct rte_eth_dev* dev,
                           struct rte_eth_rss_conf* cfg) {
  struct snemu_port* port = dev->data->dev_private;
  return feat_rss_hash_update(port, cfg);
}

static int rss_hash_conf_get(struct rte_eth_dev* dev,
                             struct rte_eth_rss_conf* cfg) {
  const struct snemu_port* port = dev->data->dev_private;
  return feat_rss_hash_conf_get(port, cfg);
}

static int reta_update(struct rte_eth_dev* dev,
                       struct rte_eth_rss_reta_entry64* conf,
                       uint16_t reta_size) {
  struct snemu_port* port = dev->data->dev_private;
  return feat_rss_reta_update(port, conf, reta_size);
}

static int reta_query(struct rte_eth_dev* dev,
                      struct rte_eth_rss_reta_entry64* conf,
                      uint16_t reta_size) {
  const struct snemu_port* port = dev->data->dev_private;
  return feat_rss_reta_query(port, conf, reta_size);
}

static int mac_addr_add_op(struct rte_eth_dev* dev,
                           struct rte_ether_addr* mac,
                           uint32_t index,
                           uint32_t vmdq) {
  (void)vmdq;
  return feat_mac_filter_add(dev->data->dev_private, mac, index);
}

static void mac_addr_remove_op(struct rte_eth_dev* dev, uint32_t index) {
  feat_mac_filter_remove(dev->data->dev_private, index);
}

static int mac_addr_set_op(struct rte_eth_dev* dev,
                           struct rte_ether_addr* mac) {
  return feat_mac_filter_set_default(dev->data->dev_private, mac);
}

static int set_mc_addr_list_op(struct rte_eth_dev* dev,
                               struct rte_ether_addr* list,
                               uint32_t nb) {
  return feat_mac_filter_set_mc_list(dev->data->dev_private, list, nb);
}

static int promiscuous_enable_op(struct rte_eth_dev* dev) {
  feat_mac_filter_set_promisc(dev->data->dev_private, true);
  return 0;
}

static int promiscuous_disable_op(struct rte_eth_dev* dev) {
  feat_mac_filter_set_promisc(dev->data->dev_private, false);
  return 0;
}

static int allmulticast_enable_op(struct rte_eth_dev* dev) {
  feat_mac_filter_set_allmulticast(dev->data->dev_private, true);
  return 0;
}

static int allmulticast_disable_op(struct rte_eth_dev* dev) {
  feat_mac_filter_set_allmulticast(dev->data->dev_private, false);
  return 0;
}

const struct eth_dev_ops PMD_ETH_OPS = {
  .dev_configure = dev_configure,
  .dev_start = dev_start,
  .dev_stop = dev_stop,
  .dev_close = dev_close,
  .dev_infos_get = dev_infos_get,
  .rx_queue_setup = rx_queue_setup,
  .tx_queue_setup = tx_queue_setup,
  .rx_queue_release = rx_queue_release,
  .tx_queue_release = tx_queue_release,
  .link_update = link_update,
  .rss_hash_update = rss_hash_update,
  .rss_hash_conf_get = rss_hash_conf_get,
  .reta_update = reta_update,
  .reta_query = reta_query,
  .mac_addr_add = mac_addr_add_op,
  .mac_addr_remove = mac_addr_remove_op,
  .mac_addr_set = mac_addr_set_op,
  .set_mc_addr_list = set_mc_addr_list_op,
  .promiscuous_enable = promiscuous_enable_op,
  .promiscuous_disable = promiscuous_disable_op,
  .allmulticast_enable = allmulticast_enable_op,
  .allmulticast_disable = allmulticast_disable_op,
};

uint16_t pmd_rx_pkt_burst(void* rx_queue, struct rte_mbuf** pkts, uint16_t nb) {
  struct snemu_rx_queue* rxq = rx_queue;
  if (rxq == nullptr || rxq->port == nullptr) {
    return 0;
  }
  uint16_t got = frontend_io_pull(rxq->port->backend, pkts, nb);
  got = emulator_node_process(rxq->port, pkts, got);
  rxq->pkts += got;
  return got;
}

uint16_t pmd_tx_pkt_burst(void* tx_queue, struct rte_mbuf** pkts, uint16_t nb) {
  struct snemu_tx_queue* txq = tx_queue;
  if (txq == nullptr || txq->port == nullptr) {
    return 0;
  }
  uint16_t sent = frontend_io_push(txq->port->backend, pkts, nb);
  txq->pkts += sent;
  return sent;
}
