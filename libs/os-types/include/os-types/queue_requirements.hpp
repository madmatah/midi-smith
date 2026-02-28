#pragma once

#include <cstdint>

namespace midismith::os {

constexpr std::uint32_t kWaitForever = 0xFFFFFFFFU;
constexpr std::uint32_t kNoWait = 0;

template <typename T>
class QueueRequirements {
 public:
  virtual ~QueueRequirements() = default;

  virtual bool Send(const T& item, std::uint32_t timeout_ms) noexcept = 0;
  virtual bool SendFromIsr(const T& item) noexcept = 0;
  virtual bool Receive(T& item, std::uint32_t timeout_ms) noexcept = 0;
};

}  // namespace midismith::os
