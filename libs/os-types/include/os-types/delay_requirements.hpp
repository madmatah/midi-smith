#pragma once

#include <cstdint>

namespace midismith::os {

class DelayRequirements {
 public:
  virtual ~DelayRequirements() = default;

  virtual void DelayMs(std::uint32_t milliseconds) noexcept = 0;
};

}  // namespace midismith::os
