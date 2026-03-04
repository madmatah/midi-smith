#pragma once

#include <cstdint>

#include "protocol-can/can_filter_set.hpp"

namespace midismith::protocol_can {

class CanFilterFactory {
 public:
  static CanFilterSet<5> MakeAdcFilters(std::uint8_t node_id) noexcept;
  static CanFilterSet<4> MakeMainFilters() noexcept;
};

}  // namespace midismith::protocol_can
