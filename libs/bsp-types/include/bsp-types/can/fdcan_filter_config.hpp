#pragma once

#include <cstdint>

namespace midismith::bsp::can {

struct FdcanFilterConfig {
  std::uint32_t filter_index;
  std::uint32_t id;
  std::uint32_t id_mask;
};

}  // namespace midismith::bsp::can
