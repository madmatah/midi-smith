#pragma once

#include <cstdint>

namespace midismith::common::os {

class ThisTask {
 public:
  static std::uint32_t stack_high_water_mark_bytes() noexcept;
};

}  // namespace midismith::common::os
