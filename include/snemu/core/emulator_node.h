#pragma once
#include <stdint.h>

struct rte_mbuf;
struct snemu_port;

uint16_t emulator_node_process(struct snemu_port* port,
                               struct rte_mbuf** pkts,
                               uint16_t nb);
