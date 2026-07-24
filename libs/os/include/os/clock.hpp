#pragma once

#include <cstdint>

namespace midismith::os {

class Clock {
 public:
  static void delay_ms(std::uint32_t ms) noexcept;
  static std::uint32_t now_ms() noexcept;
};

}  // namespace midismith::os
