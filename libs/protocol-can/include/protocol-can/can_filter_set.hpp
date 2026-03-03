#pragma once

#include <array>
#include <cstddef>

#include "bsp-types/can/fdcan_filter_config.hpp"

namespace midismith::protocol_can {

template <std::size_t N>
struct CanFilterSet {
  std::array<midismith::bsp::can::FdcanFilterConfig, N> filters;
};

}  // namespace midismith::protocol_can
