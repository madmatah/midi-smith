#pragma once

#include <cstdint>

namespace midismith::os {

class Clock {
 public:
  static void delay_ms(std::uint32_t ms) noexcept;
};

}  // namespace midismith::os
