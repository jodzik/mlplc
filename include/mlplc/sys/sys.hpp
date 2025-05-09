#pragma once

#include <mlplc/sys/mutex.hpp>
#include <mlplc/sys/thread.hpp>

#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/sys_heap.h>

#include <chrono>

namespace mlplc {
namespace sys {

static inline void sleep(std::chrono::milliseconds ms) {
    k_sleep(K_MSEC(ms.count()));
}

static inline std::chrono::milliseconds uptime() {
    return std::chrono::milliseconds(k_uptime_get());
}

static inline void reset() {
    LOG_PANIC();
    sys_reboot(SYS_REBOOT_WARM);
    sys_reboot(SYS_REBOOT_COLD);
}

std::size_t mem_usage();

namespace _private {

}
}
}                         