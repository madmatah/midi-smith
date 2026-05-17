#pragma once

#include <cstdint>

namespace midismith::os {

class BinarySemaphoreRequirements {
 public:
  virtual ~BinarySemaphoreRequirements() = default;

  virtual bool Acquire(std::uint32_t timeout_ms) noexcept = 0;
  virtual bool Release() noexcept = 0;
};

}  // namespace midismith::os
