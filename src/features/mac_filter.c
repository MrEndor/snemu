#include <errno.h>
#include <rte_ether.h>
#include <snemu/features/mac_filter.h>
#include <snemu/port.h>
#include <stddef.h>
#include <string.h>

int feat_mac_filter_init(struct snemu_port* port,
                         const struct rte_ether_addr* default_addr) {
  if (port == nullptr || default_addr == nullptr) {
    return -EINVAL;
  }
  struct snemu_mac_filter_ctx* ctx = &port->mac_filter;
  memset(ctx, 0, sizeof(*ctx));
  rte_ether_addr_copy(default_addr, &ctx->unicast[0]);
  ctx->used[0] = true;
  return 0;
}

int feat_mac_filter_add(struct snemu_port* port,
                        const struct rte_ether_addr* mac,
                        uint32_t index) {
  if (port == nullptr || mac == nullptr || index >= SNEMU_MAX_UC_MACS) {
    return -EINVAL;
  }
  rte_ether_addr_copy(mac, &port->mac_filter.unicast[index]);
  port->mac_filter.used[index] = true;
  return 0;
}

void feat_mac_filter_remove(struct snemu_port* port, uint32_t index) {
  if (port == nullptr || index == 0 || index >= SNEMU_MAX_UC_MACS) {
    return;
  }
  port->mac_filter.used[index] = false;
}

int feat_mac_filter_set_default(struct snemu_port* port,
                                const struct rte_ether_addr* mac) {
  if (port == nullptr || mac == nullptr) {
    return -EINVAL;
  }
  rte_ether_addr_copy(mac, &port->mac_filter.unicast[0]);
  port->mac_filter.used[0] = true;
  return 0;
}

int feat_mac_filter_set_mc_list(struct snemu_port* port,
                                const struct rte_ether_addr* list,
                                uint32_t nb) {
  if (port == nullptr || nb > SNEMU_MAX_MC_MACS) {
    return -EINVAL;
  }
  if (nb > 0 && list == nullptr) {
    return -EINVAL;
  }
  for (uint32_t i = 0; i < nb; i++) {
    rte_ether_addr_copy(&list[i], &port->mac_filter.multicast[i]);
  }
  port->mac_filter.mc_count = nb;
  return 0;
}

void feat_mac_filter_set_promisc(struct snemu_port* port, bool on) {
  if (port == nullptr) {
    return;
  }
  port->mac_filter.promisc = on;
}

void feat_mac_filter_set_allmulticast(struct snemu_port* port, bool on) {
  if (port == nullptr) {
    return;
  }
  port->mac_filter.allmulticast = on;
}

bool feat_mac_filter_match(const struct snemu_port* port,
                           const struct rte_ether_addr* dmac) {
  if (port == nullptr || dmac == nullptr) {
    return false;
  }
  const struct snemu_mac_filter_ctx* ctx = &port->mac_filter;

  if (ctx->promisc) {
    return true;
  }
  if (rte_is_broadcast_ether_addr(dmac)) {
    return true;
  }
  if (rte_is_multicast_ether_addr(dmac)) {
    if (ctx->allmulticast) {
      return true;
    }
    for (uint32_t i = 0; i < ctx->mc_count; i++) {
      if (rte_is_same_ether_addr(dmac, &ctx->multicast[i])) {
        return true;
      }
    }
    return false;
  }
  for (uint32_t i = 0; i < SNEMU_MAX_UC_MACS; i++) {
    if (ctx->used[i] && rte_is_same_ether_addr(dmac, &ctx->unicast[i])) {
      return true;
    }
  }
  return false;
}
