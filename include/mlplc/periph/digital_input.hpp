#pragma once

#include <mlplc/exception.hpp>
#include <mlplc/sys/sys.hpp>
#include <mlplc/periph/digital_common.hpp>
#include "dtb.hpp"

#include <zephyr/device.h>

#include <memory>
#include <optional>
#include <chrono>
#include <string_view>

namespace mlplc {
namespace periph {

using namespace std::chrono_literals;

enum class InputPull {
    None,
    Up,
    Down,
};

struct LevelDuration {
    std::chrono::milliseconds duration;
    Level level;
};

class DigitalInput {
public:
    DigitalInput(uint8_t port, InputPull pull = InputPull::Up, std::chrono::milliseconds debounce_duration = 100ms);
    DigitalInput(
        std::string_view label,
        InputPull pull = InputPull::Up,
        std::chrono::milliseconds debounce_duration = 100ms);

    ~DigitalInput();

    Level level() const;

    void wait_level(Level level) const;

    std::optional<Level> is_level_changed() const;

    LevelDuration level_duration() const;

    std::optional<Level> level_debounced() const;

    void wait_level_debounced(Level level) const;

    std::optional<Level> is_level_changed_debounced() const;

    void set_debounce_duration(std::chrono::milliseconds debounce_duration);

private:
    mutable sys::Mutex _mutex;
    uint8_t _port;
    InputPull _pull;
    std::chrono::milliseconds _debounce_duration;
    dtb::gpio_spec_t _spec;

    mutable Level _prev_level;
    mutable Level _prev_level_debounced;
    mutable bool _is_level_changed = false;
    mutable bool _is_level_debounced_changed = false;
    mutable std::chrono::milliseconds _tl_level_change = 0ms;

    Level _level() const;
    LevelDuration _level_duration() const;
    std::optional<Level> _level_debounced() const;
};

} // namespace periph
} // namespace mlplc
