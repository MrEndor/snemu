#include "ethdev_pnp.h"

#include <bus_vdev_driver.h>
#include <ethdev_driver.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_log.h>
#include <rte_malloc.h>

#include "snemu/core/frontend_io.h"
#include "snemu/features/mac_filter.h"
#include "snemu/log.h"
#include "snemu/port.h"

RTE_LOG_REGISTER_DEFAULT(snemu_logtype, INFO)

static int snemu_probe(struct rte_vdev_device* vdev) {
  const char* name = rte_vdev_device_name(vdev);

  struct rte_eth_dev* eth_dev = rte_eth_dev_allocate(name);
  if (eth_dev == nullptr) {
    SNEMU_LOG_ERR("rte_eth_dev_allocate failed for %s", name);
    return -ENOSPC;
  }

  struct snemu_port* port =
    rte_zmalloc_socket(name, sizeof(*port), 0, vdev->device.numa_node);
  if (port == nullptr) {
    rte_eth_dev_release_port(eth_dev);
    return -ENOMEM;
  }

  struct rte_ether_addr* mac_addrs = rte_zmalloc_socket(
    "snemu_macs",
    sizeof(struct rte_ether_addr) * SNEMU_MAX_UC_MACS,
    0,
    vdev->device.numa_node);
  if (mac_addrs == nullptr) {
    rte_free(port);
    rte_eth_dev_release_port(eth_dev);
    return -ENOMEM;
  }
  rte_eth_random_addr(mac_addrs[0].addr_bytes);
  feat_mac_filter_init(port, &mac_addrs[0]);

  port->eth_dev = eth_dev;
  port->backend = null_backend();

  eth_dev->data->dev_private = port;
  eth_dev->data->numa_node = vdev->device.numa_node;
  eth_dev->data->mac_addrs = mac_addrs;
  eth_dev->device = &vdev->device;
  eth_dev->dev_ops = &PMD_ETH_OPS;
  eth_dev->rx_pkt_burst = pmd_rx_pkt_burst;
  eth_dev->tx_pkt_burst = pmd_tx_pkt_burst;

  rte_eth_dev_probing_finish(eth_dev);
  SNEMU_LOG_INFO("probed %s", name);
  return 0;
}

static int snemu_remove(struct rte_vdev_device* vdev) {
  const char* name = rte_vdev_device_name(vdev);
  struct rte_eth_dev* eth_dev = rte_eth_dev_allocated(name);
  if (eth_dev == nullptr) {
    return 0;
  }

  rte_free(eth_dev->data->mac_addrs);
  eth_dev->data->mac_addrs = nullptr;
  rte_free(eth_dev->data->dev_private);
  rte_eth_dev_release_port(eth_dev);
  SNEMU_LOG_INFO("removed %s", name);
  return 0;
}

static struct rte_vdev_driver snemu_drv = {
  .probe = snemu_probe,
  .remove = snemu_remove,
};

RTE_PMD_REGISTER_VDEV(net_snemu, snemu_drv);
