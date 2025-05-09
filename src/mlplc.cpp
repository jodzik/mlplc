#include <mlplc/mlplc.hpp>

#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include <iostream>

namespace mlplc {

LOG_MODULE_REGISTER(mlplc);

namespace {

std::function<void(std::string const&, FatalReason)> _on_fatal_error;

constexpr FatalReason fatal_reason_from_int(unsigned int reason) {
    switch (reason) {
    default: return FatalReason::Unknown;
    }
}

extern "C" {
void k_sys_fatal_error_handler(unsigned int	reason, const struct arch_esf *	esf) {
    FatalReason const fatal_reason = fatal_reason_from_int(reason);
    k_tid_t tid = k_current_get();
    LOG_ERR("reason=%u", reason);
    if (tid) {
        char const* const name = k_thread_name_get(tid);
        if (name) {
            _on_fatal_error(std::string(name), fatal_reason);
        } else {
            _on_fatal_error("", fatal_reason);
        }
    } else {
        LOG_ERR("Fail to get crashed thread.");
        _on_fatal_error("", fatal_reason);
    }
    LOG_PANIC();
    sys_reboot(SYS_REBOOT_WARM);
    sys_reboot(SYS_REBOOT_COLD);
}
}

}

void init(std::function<void(std::string const&, FatalReason)> on_fatal_error) {
    _on_fatal_error = on_fatal_error;
}

} // namespace mlplc
