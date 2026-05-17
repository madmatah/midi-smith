#pragma once

#include <cstdint>

#include "os-types/binary_semaphore_requirements.hpp"

namespace midismith::os {

class BinarySemaphore : public BinarySemaphoreRequirements {
 public:
  BinarySemaphore() noexcept;
  ~BinarySemaphore() noexcept override;

  BinarySemaphore(const BinarySemaphore&) = delete;
  BinarySemaphore& operator=(const BinarySemaphore&) = delete;

  bool Acquire(std::uint32_t timeout_ms) noexcept override;
  bool Release() noexcept override;

 private:
  void* _sem;
};

}  // namespace midismith::os
