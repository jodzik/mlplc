#pragma once

#include <mlplc/exception.hpp>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <optional>
#include <string>
#include <cstdint>
#include <expected>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string_view>

namespace mlplc {
namespace dtb {

namespace _private {

};

using ports_t = uint64_t;

constexpr std::size_t GPIO_COUNT = DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(mlplc_gpios));

constexpr ports_t ports_mask(std::initializer_list<uint8_t const> ports_list) {
    ports_t ports_mask = 0;

    for (auto p : ports_list) {
        ports_mask |= 1 << p;
    }

    return ports_mask;
}

enum class DeviceType : uint8_t {
    Gpio,
    Serial,
};

struct DeviceId {
    DeviceType type;
    uint8_t idx;
};

template <typename T>
class Deleter {
public:
    Deleter(DeviceId dev_id, std::function<void(DeviceId)> drop_cb) :
        _dev_id(dev_id),
        _drop_cb(drop_cb)
    {}

    ~Deleter() {}

    void operator() ([[maybe_unused]] T const* dt_spec) {
        this->_drop_cb(this->_dev_id);
    }

private:
    DeviceId _dev_id;
    std::function<void(DeviceId)> _drop_cb;
};

using gpio_spec_t = std::unique_ptr<struct gpio_dt_spec const, Deleter<struct gpio_dt_spec>>;


std::optional<DeviceId> port_owner(uint8_t port);

std::optional<DeviceId> dev_that_ports_overlap_with(ports_t ports);

std::optional<ports_t> dev_ports_in_use(DeviceId dev_id);

bool is_dev_in_use(DeviceId dev_id);

gpio_spec_t borrow_gpio(uint8_t idx);
uint8_t find_gpio_idx_by_label(std::string_view label);

} // namespace dtb
} // namespace mlplc
