#pragma once

#include <rte_ether.h>
#include <stdbool.h>
#include <stdint.h>

struct snemu_port;

#define SNEMU_MAX_UC_MACS 128
#define SNEMU_MAX_MC_MACS 128

struct snemu_mac_filter_ctx {
  bool promisc;
  bool allmulticast;
  // Slot 0 is the primary/default unicast MAC; remaining slots are
  // populated by mac_addr_add. used[i] gates whether slot i is active.
  bool used[SNEMU_MAX_UC_MACS];
  struct rte_ether_addr unicast[SNEMU_MAX_UC_MACS];

  uint32_t mc_count;
  struct rte_ether_addr multicast[SNEMU_MAX_MC_MACS];
};

int feat_mac_filter_init(struct snemu_port* port,
                         const struct rte_ether_addr* default_addr);

int feat_mac_filter_add(struct snemu_port* port,
                        const struct rte_ether_addr* mac,
                        uint32_t index);

void feat_mac_filter_remove(struct snemu_port* port, uint32_t index);

int feat_mac_filter_set_default(struct snemu_port* port,
                                const struct rte_ether_addr* mac);

int feat_mac_filter_set_mc_list(struct snemu_port* port,
                                const struct rte_ether_addr* list,
                                uint32_t nb);

void feat_mac_filter_set_promisc(struct snemu_port* port, bool on);

void feat_mac_filter_set_allmulticast(struct snemu_port* port, bool on);

bool feat_mac_filter_match(const struct snemu_port* port,
                           const struct rte_ether_addr* dmac);
