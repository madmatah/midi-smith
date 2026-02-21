#pragma once

#include <cstdint>

namespace os {

class Clock {
 public:
  static void delay_ms(std::uint32_t ms) noexcept;
};

}  // namespace os

namespace midismith::common::os {
using ::os::Clock;
}
