#include <mlplc/periph/digital_input.hpp>
#include "dtb.hpp"

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/sys/printk.h>

#include <mutex>

namespace mlplc {
namespace periph {

using namespace std::chrono_literals;


DigitalInput::DigitalInput(uint8_t port, InputPull pull, std::chrono::milliseconds debounce_duration) :
    _port(port),
    _pull(pull),
    _debounce_duration(debounce_duration),
    _spec(dtb::borrow_gpio(port))
{
    gpio_flags_t flags = 0;
    if (InputPull::Down == this->_pull) {
        flags = GPIO_PULL_DOWN;
    } else if (InputPull::Up == this->_pull) {
        flags = GPIO_PULL_UP;
    }
    CCALL(gpio_pin_configure(this->_spec->port, this->_spec->pin, GPIO_INPUT | flags));
    this->_prev_level = this->level();
    this->_prev_level_debounced = this->_prev_level;
}

DigitalInput::DigitalInput(
    std::string_view label,
    InputPull pull,
    std::chrono::milliseconds debounce_duration) :
    DigitalInput(dtb::find_gpio_idx_by_label(label), pull, debounce_duration) {}

DigitalInput::~DigitalInput() {
    gpio_pin_configure(this->_spec->port, this->_spec->pin, DEINIT_GPIO_MODE);
}

Level DigitalInput::level() const {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    return this->_level();
}

Level DigitalInput::_level() const {
    Level const new_level = level_from_num(gpio_pin_get_dt(this->_spec.get()));
    if (new_level != this->_prev_level) {
        this->_tl_level_change = sys::uptime();
        this->_prev_level = new_level;
        this->_is_level_changed = true;
    }
    return new_level;
}

void DigitalInput::wait_level(Level level) const {
    while (this->level() != level) {
        sys::sleep(GPIO_WAIT);
    }
}

std::optional<Level> DigitalInput::is_level_changed() const {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    Level const new_level = this->_level();
    if (this->_is_level_changed) {
        this->_is_level_changed = false;
        return new_level;
    }

    return std::nullopt;
}

LevelDuration DigitalInput::level_duration() const {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    return this->_level_duration();
}

LevelDuration DigitalInput::_level_duration() const {
    Level const current_level = this->_level();
    auto const now = sys::uptime();
    return LevelDuration {.duration = now - this->_tl_level_change, .level = current_level};
}

std::optional<Level> DigitalInput::level_debounced() const {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    return this->_level_debounced();
}

std::optional<Level> DigitalInput::_level_debounced() const {
    auto const level_duration = this->_level_duration();
    if (level_duration.duration >= this->_debounce_duration) {
        if (this->_prev_level_debounced != level_duration.level) {
            this->_prev_level_debounced = level_duration.level;
            this->_is_level_debounced_changed = true;
        }
        return level_duration.level;
    }
    return std::nullopt;
}

void DigitalInput::wait_level_debounced(Level level) const {
    while (this->level_debounced() != level) {
        sys::sleep(GPIO_WAIT);
    }
}

std::optional<Level> DigitalInput::is_level_changed_debounced() const {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    auto new_level = this->_level_debounced();
    if (new_level && this->_is_level_debounced_changed) {
        this->_is_level_debounced_changed = false;
        return new_level;
    }
    return std::nullopt;
}

void DigitalInput::set_debounce_duration(std::chrono::milliseconds debounce_duration) {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    this->_debounce_duration = debounce_duration;
}

} // namespace periph
} // namespace mlplc
