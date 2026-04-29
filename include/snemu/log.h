#pragma once

#include <rte_log.h>

extern int snemu_logtype;

#define SNEMU_LOG(level, fmt, ...) \
  rte_log(RTE_LOG_##level,         \
          snemu_logtype,           \
          "[snemu][%s] " fmt,      \
          __func__,                \
          ##__VA_ARGS__)

#define SNEMU_LOG_ERR(fmt, ...)  SNEMU_LOG(ERR, fmt "\n", ##__VA_ARGS__)
#define SNEMU_LOG_WARN(fmt, ...) SNEMU_LOG(WARNING, fmt "\n", ##__VA_ARGS__)
#define SNEMU_LOG_INFO(fmt, ...) SNEMU_LOG(INFO, fmt "\n", ##__VA_ARGS__)
#define SNEMU_LOG_DBG(fmt, ...)  SNEMU_LOG(DEBUG, fmt "\n", ##__VA_ARGS__)
