/** Device Tree Base. Module for manage/borrow devices. @file
 * When device borrowing requested, a some checks occured:
 * 1. Device is not already borrowed;
 * 2. Ports/pins used by requested device is not already in use.
 * 
 * Circumstances:
 * - Many MCU pins may be connected to one port.
 * - Device may use pins, which not connected to any port.
 * - Device may not use any pins.
 * 
 * When device borrowing request:
 * - Check for device is not already used.
 * - Check for all device ports is not already used.
 * - Get raw device from device tree.
 * - Mark requested device as used.
 * - Mark used by device ports as used.
 * 
 * When device dropped:
 * - Mark device as unused.
 * - Mark used by device ports as unused.
 */

#include "dtb.hpp"
#include "macro.hpp"
#include <mlplc/sys/mutex.hpp>

#include <array>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

template <>
struct std::hash<mlplc::dtb::DeviceId>
{
    std::size_t operator()(mlplc::dtb::DeviceId const& self) const {
        uint64_t const raw = (static_cast<uint8_t>(self.type) << 8) | self.idx;
        return static_cast<std::size_t>(raw);
    }
};

namespace mlplc {
namespace dtb {

bool operator== (DeviceId const& self, DeviceId const& other) {
    return self.idx == other.idx && self.type == other.type;
}

std::ostream& operator<< (std::ostream& os, DeviceId const& dev_id) {
    os << magic_enum::enum_name(dev_id.type) << "_" << static_cast<int>(dev_id.idx);
    return os;
}

namespace {

template <typename T>
struct DtSpec {
    T dt_spec;
    uint8_t idx;
    std::string_view label;
};

std::unordered_map<DeviceId, ports_t> const DEVICE_PORTS_MAP = {
    // TODO:
    {{DeviceType::Gpio, 0}, ports_mask({0})},
};

#define PRINT_GPIO_DT_SPEC(node_id) DtSpec {gpio_dt_spec GPIO_DT_SPEC_GET(node_id, gpios), DT_PROP(node_id, idx), DT_PROP(node_id, label)},

std::array<DtSpec<struct gpio_dt_spec>, GPIO_COUNT> const GPIOS = {
    DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(mlplc_gpios), PRINT_GPIO_DT_SPEC)
};

static sys::Mutex mutex;
std::unordered_map<DeviceId, bool> device_in_use;

void assert_device_borrowing(DeviceId dev_id) {
    ASSERT(!is_dev_in_use(dev_id), ExceptionType::DeviceAlreadyInUse, dev_id);

    // Check ports overlapping.
    if (DEVICE_PORTS_MAP.contains(dev_id)) {
        // If device exists in device-ports map - this mean that it may use ports.
        auto overlapped_dev = dev_that_ports_overlap_with(DEVICE_PORTS_MAP.at(dev_id));
        ASSERT(!overlapped_dev, ExceptionType::PortAlreadyInUse, *overlapped_dev);
    }
}

void on_dev_drop(DeviceId dev_id) {
    std::lock_guard<sys::Mutex> lock(mutex);
    if (device_in_use.contains(dev_id)) {
        device_in_use[dev_id] = false;
    }
}

template <typename T, std::size_t N>
std::optional<DtSpec<T>> find_dt_spec_by_idx(std::array<DtSpec<T>, N> const& array, uint8_t idx) {
    for (auto const& d : array) {
        if (d.idx == idx) {
            return d;
        }
    }
    return std::nullopt;
}

template <typename T, std::size_t N>
std::optional<uint8_t> find_idx_by_label(std::array<DtSpec<T>, N> const& array, std::string_view label) {
    for (auto const& d : array) {
        if (d.label == label) {
            return d.idx;
        }
    }
    return std::nullopt;
}

}


std::optional<DeviceId> port_owner(uint8_t port) {
    ports_t const port_mask = ports_mask({port});

    return dev_that_ports_overlap_with(port_mask);
}

std::optional<DeviceId> dev_that_ports_overlap_with(ports_t ports_mask) {
    std::lock_guard<sys::Mutex> lock(mutex);
    for (auto const& [dev_id, is_used] : device_in_use) {
        if (is_used) {
            ports_t const dev_ports = DEVICE_PORTS_MAP.at(dev_id);
            if (dev_ports & ports_mask) {
                return dev_id;
            }
        }
    }

    return std::nullopt;
}

std::optional<ports_t> dev_ports_in_use(DeviceId dev_id) {
    std::lock_guard<sys::Mutex> lock(mutex);
    if (device_in_use.contains(dev_id) && device_in_use.at(dev_id)) {
        return DEVICE_PORTS_MAP.at(dev_id);
    }

    return std::nullopt;
}

bool is_dev_in_use(DeviceId dev_id) {
    std::lock_guard<sys::Mutex> lock(mutex);
    return device_in_use.contains(dev_id) && device_in_use.at(dev_id);
}

gpio_spec_t borrow_gpio(uint8_t idx) {
    DeviceId const dev_id = DeviceId{DeviceType::Gpio, idx};
    assert_device_borrowing(dev_id);
    auto result = find_dt_spec_by_idx<struct gpio_dt_spec, GPIO_COUNT>(GPIOS, idx);
    ASSERT(result, ExceptionType::NoDev, "Gpio not found: ", idx);
    return gpio_spec_t(new gpio_dt_spec(result->dt_spec), Deleter<struct gpio_dt_spec>(dev_id, on_dev_drop));
}

uint8_t find_gpio_idx_by_label(std::string_view label) {
    auto const result = find_idx_by_label<struct gpio_dt_spec, GPIO_COUNT>(GPIOS, label);
    ASSERT(result, ExceptionType::NoDev, label);
    return *result;
}

namespace _private {

};

}
}
