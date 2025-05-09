#include <mlplc/periph/digital_output.hpp>
#include "dtb.hpp"

#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree/gpio.h>

#include <mutex>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace mlplc {
namespace periph {

using namespace std::chrono_literals;

namespace {

constexpr std::size_t GPIO_HANDLER_STACK_SIZE = 2048;
constexpr int8_t GPIO_HANDLER_PRIO = 1;
constexpr uint32_t GPIO_HANDLER_OPTIONS = K_ESSENTIAL;
constexpr std::chrono::microseconds GPIO_HANDLER_LOCK_TIMEOUT = 100us;

sys::Mutex g_gpio_handlers_mutex;
std::unordered_map<uint8_t, std::function<void(std::chrono::milliseconds)>> g_gpio_handlers;

void gpio_handler(void* arg1, void* arg2, void* arg3) {
    while (1) {
        {
            std::lock_guard<sys::Mutex> lock(g_gpio_handlers_mutex);
            auto const now = sys::uptime();
            for (auto const& [p, f] : g_gpio_handlers) {
                f(now);
            }
        }
        sys::sleep(GPIO_WAIT);
    }
}

K_KERNEL_THREAD_DEFINE(gpio_handler_thread, GPIO_HANDLER_STACK_SIZE, gpio_handler, NULL, NULL, NULL,
    GPIO_HANDLER_PRIO, GPIO_HANDLER_OPTIONS, 0);

} // namespace

DigitalOutput::DigitalOutput(uint8_t port, OutputType type, Level level) :
    _port(port),
    _type(type),
    _desired_level(level),
    _spec(dtb::borrow_gpio(port))
{
    gpio_flags_t flags = 0;
    if (OutputType::PushPull == this->_type) {
        flags = GPIO_PUSH_PULL;
    } else if (OutputType::OpenDrain == this->_type) {
        flags = GPIO_OPEN_DRAIN;
    }

    CCALL(gpio_pin_configure(this->_spec->port, this->_spec->pin, GPIO_OUTPUT | flags));
    CCALL(gpio_pin_set(this->_spec->port, this->_spec->pin, level_to_int(level)));

    std::lock_guard<sys::Mutex> lock(g_gpio_handlers_mutex);
    g_gpio_handlers[port] = std::bind(&DigitalOutput::_handle, this, std::placeholders::_1);
}

DigitalOutput::DigitalOutput(std::string_view label, OutputType type, Level level) :
    DigitalOutput(dtb::find_gpio_idx_by_label(label), type, level) {}

DigitalOutput::~DigitalOutput() {
    std::lock_guard<sys::Mutex> lock(g_gpio_handlers_mutex);
    g_gpio_handlers.erase(this->_port);
    gpio_pin_configure(this->_spec->port, this->_spec->pin, DEINIT_GPIO_MODE);
}

void DigitalOutput::set_level(Level level) {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    this->_desired_level = level;
    this->_set_level(level);
}

void DigitalOutput::_set_level(Level level) noexcept {
    CCALL_UNTIL(gpio_pin_set(this->_spec->port, this->_spec->pin, level_to_int(level)));
}

void DigitalOutput::pulse_start(std::chrono::milliseconds t_low, std::chrono::milliseconds t_high,
    std::optional<std::size_t> count, Level first_level)
{
    if (!count || *count > 0) {
        std::lock_guard<sys::Mutex> lock(this->_mutex);
        this->_pulse_t_low = t_low;
        this->_pulse_t_high = t_high;
        if (count) {
            this->_pulse_remain = *count;
            this->_is_pulse_continuous = false;
        } else {
            this->_is_pulse_continuous = true;
        }
        this->_pulse_first_level = first_level;
        this->_pulse_state = first_level;
        this->_set_level(first_level);
        this->_tl_pulse_sw = sys::uptime();
    }
}

void DigitalOutput::pulse_stop() {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    this->_pulse_state = std::nullopt;
    this->_set_level(this->_desired_level);
}

void DigitalOutput::wait_pulse_end() const {
    while (this->is_pulse_run()) {
        sys::sleep(GPIO_WAIT);
    }
}

bool DigitalOutput::is_pulse_run() const {
    std::lock_guard<sys::Mutex> lock(this->_mutex);
    return this->_pulse_state.has_value();
}

void DigitalOutput::_handle(std::chrono::milliseconds now) noexcept {
    if (this->_mutex.try_lock(GPIO_HANDLER_LOCK_TIMEOUT)) {
        if (this->_pulse_state.has_value()) {
            std::chrono::milliseconds duration = this->_pulse_t_low;
            if (Level::High == this->_pulse_state) {
                duration = this->_pulse_t_high;
            }
            bool const is_time_to_sw = (now - this->_tl_pulse_sw) >= duration;
            if (is_time_to_sw) {
                if (!this->_is_pulse_continuous && this->_pulse_state != this->_pulse_first_level) {
                    this->_pulse_remain -= 1;
                }
                if (!this->_is_pulse_continuous && 0 == this->_pulse_remain) {
                    this->_pulse_state = std::nullopt;
                } else {
                    this->_tl_pulse_sw = now;
                    this->_pulse_state = !*this->_pulse_state;
                    this->_set_level(*this->_pulse_state);
                }
            }
        }

        this->_mutex.unlock();
    }
}

}
}