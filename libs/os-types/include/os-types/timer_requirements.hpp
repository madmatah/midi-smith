#pragma once

#include <cstdint>

namespace midismith::os {

class TimerRequirements {
 public:
  virtual ~TimerRequirements() = default;

  virtual bool Start(std::uint32_t period_ms) noexcept = 0;
  virtual bool Stop() noexcept = 0;
};

}  // namespace midismith::os
