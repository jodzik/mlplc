#pragma once

#include "sys/sys.hpp"
#include "periph/digital_input.hpp"
#include "periph/digital_output.hpp"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <functional>
#include <string>

namespace mlplc {

enum class FatalReason {
    Unknown,
};

constexpr char const* fatal_reason_to_str(FatalReason reason) {
    return "Unknown";
}

void init(std::function<void(std::string const&, FatalReason)> on_fatal_error);

}
