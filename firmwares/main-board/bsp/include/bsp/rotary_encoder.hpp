#pragma once

#include <cstdint>

#include "bsp-types/input/rotation_source_requirements.hpp"

namespace midismith::main_board::bsp {

class RotaryEncoder final : public midismith::bsp::input::RotationSourceRequirements {
 public:
  explicit RotaryEncoder(void* timer_handle) noexcept;

  void Start() noexcept override;
  std::int16_t ReadDeltaDetents() noexcept override;
  std::uint16_t raw_counter() const noexcept override;

 private:
  static constexpr std::int16_t kCountsPerDetent = 4;

  void* timer_handle_;
  std::uint16_t previous_counter_ = 0;
  std::int16_t pending_counts_ = 0;
};

}  // namespace midismith::main_board::bsp
