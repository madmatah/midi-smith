#pragma once

#include <cstdint>

namespace midismith::common::os {

class Memory {
 public:
  static std::uint32_t free_heap_bytes() noexcept;
  static std::uint32_t min_ever_free_heap_bytes() noexcept;
};

}  // namespace midismith::common::os
