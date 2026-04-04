#pragma once

#include <cstdint>

namespace midismith::adc_board::app::shell {

class BlockingDelayRequirements {
 public:
  virtual ~BlockingDelayRequirements() = default;

  virtual void DelayMs(std::uint32_t delay_ms) noexcept = 0;
};

}  // namespace midismith::adc_board::app::shell
