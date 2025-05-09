#pragma once

#include "macro.hpp"

#include <zephyr/kernel.h>

#include <chrono>

namespace mlplc {
namespace sys {

class Mutex {
public:
    Mutex() {
        CCALL_UNTIL(k_mutex_init(&this->_k_mutex));
    }

    void lock() {
        CCALL_UNTIL(k_mutex_lock(&this->_k_mutex, K_FOREVER));
    }

    bool try_lock(std::chrono::microseconds timeout) {
        int const rc = k_mutex_lock(&this->_k_mutex, K_USEC(timeout.count()));
        return 0 == rc;
    }

    void unlock() {
        CCALL_UNTIL(k_mutex_unlock(&this->_k_mutex));
    }

    Mutex(Mutex const&) = delete;
    Mutex(Mutex&&) = delete;

private:
    struct k_mutex _k_mutex{};
};

} // namespace sys
} // namespace mlplc