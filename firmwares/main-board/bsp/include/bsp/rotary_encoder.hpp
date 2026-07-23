#pragma once

#include <cstdint>

namespace midismith::main_board::bsp {

class RotaryEncoder {
 public:
  explicit RotaryEncoder(void* timer_handle) noexcept;

  void Start() noexcept;
  std::int16_t ReadDeltaDetents() noexcept;

 private:
  static constexpr std::int16_t kCountsPerDetent = 4;

  void* timer_handle_;
  std::uint16_t previous_counter_ = 0;
  std::int16_t pending_counts_ = 0;
};

}  // namespace midismith::main_board::bsp
