#pragma once

// #include <magic_enum/magic_enum.hpp>

#include <zephyr/drivers/gpio.h>

#include <chrono>
#include <ostream>

namespace mlplc {
namespace periph {

using namespace std::chrono_literals;

enum class Level {
    Low,
    High,
};

static inline std::ostream& operator<< (std::ostream& os, Level level) {
    if (Level::Low == level) {
        os << "LOW";
    } else {
        os << "HIGH";
    }

    return os;
}

static inline Level operator! (Level level) {
    return Level::Low == level ? Level::High : Level::Low;
}

constexpr gpio_flags_t DEINIT_GPIO_MODE = GPIO_DISCONNECTED;
constexpr std::chrono::milliseconds GPIO_WAIT = 4ms;

} // namespace periph

template <typename T>
constexpr periph::Level level_from_num(T t) {
    if (t > 0) {
        return periph::Level::High;
    } else {
        return periph::Level::Low;
    }
}

constexpr int level_to_int(periph::Level level) {
    return periph::Level::High == level ? 1 : 0;
}

constexpr const char* level_to_str(periph::Level level) {
    if (periph::Level::Low == level) {
        return "LOW";
    } else {
        return "HIGH";
    }
}

} // namespace mlplc