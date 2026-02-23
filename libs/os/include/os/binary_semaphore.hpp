#pragma once

#include <cstdint>

namespace midismith::os {

class BinarySemaphore {
 public:
  BinarySemaphore() noexcept;
  ~BinarySemaphore() noexcept;

  BinarySemaphore(const BinarySemaphore&) = delete;
  BinarySemaphore& operator=(const BinarySemaphore&) = delete;

  bool acquire(std::uint32_t timeout_ms) noexcept;
  bool release() noexcept;

 private:
  void* _sem;
};

}  // namespace midismith::os
