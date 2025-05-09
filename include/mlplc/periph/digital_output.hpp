#pragma once

#include <mlplc/exception.hpp>
#include <mlplc/sys/sys.hpp>
#include <mlplc/periph/digital_common.hpp>
#include "dtb.hpp"

#include <expected>
#include <memory>
#include <optional>
#include <chrono>
#include <string_view>

namespace mlplc {
namespace periph {

using namespace std::chrono_literals;

enum class OutputType {
    PushPull,
    OpenDrain,
};

class DigitalOutput {
public:
    DigitalOutput(uint8_t port, OutputType type = OutputType::PushPull, Level level = Level::Low);
    DigitalOutput(std::string_view label, OutputType type = OutputType::PushPull, Level level = Level::Low);

    ~DigitalOutput();

    void set_level(Level level);

    void pulse_start(std::chrono::milliseconds t_low, std::chrono::milliseconds t_high,
        std::optional<std::size_t> count = std::nullopt, Level first_level = Level::Low);

    void pulse_stop();

    void wait_pulse_end() const;

    bool is_pulse_run() const;

private:
    mutable sys::Mutex _mutex;
    uint8_t _port;
    OutputType _type;
    Level _desired_level;
    dtb::gpio_spec_t _spec;
    std::optional<Level> _pulse_state;
    std::chrono::milliseconds _tl_pulse_sw = 0ms;
    Level _pulse_first_level = Level::Low;
    std::size_t _pulse_remain = 0;
    bool _is_pulse_continuous = false;
    std::chrono::milliseconds _pulse_t_low = 0ms;
    std::chrono::milliseconds _pulse_t_high = 0ms;

    void _set_level(Level level) noexcept;

    void _handle(std::chrono::milliseconds now) noexcept;
};

} // namespace periph
} // namespace mlplc
