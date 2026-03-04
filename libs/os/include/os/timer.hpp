#pragma once

#include <cstdint>

namespace midismith::os {

class Timer {
 public:
  using Callback = void (*)(void* ctx) noexcept;

  Timer(Callback callback, void* ctx) noexcept;
  ~Timer() noexcept;

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  bool Start(std::uint32_t period_ms) noexcept;
  bool Stop() noexcept;

 private:
  void* id_;
};

}  // namespace midismith::os
