#pragma once

#include <cstdint>

#include "os-types/timer_requirements.hpp"

namespace midismith::os {

class Timer : public TimerRequirements {
 public:
  using Callback = void (*)(void* ctx) noexcept;

  Timer(Callback callback, void* ctx) noexcept;
  ~Timer() noexcept;

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  bool Start(std::uint32_t period_ms) noexcept override;
  bool Stop() noexcept override;

 private:
  void* id_;
};

}  // namespace midismith::os
