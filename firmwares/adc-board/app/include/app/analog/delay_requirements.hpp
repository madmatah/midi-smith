#pragma once

#include <cstdint>

namespace midismith::adc_board::app::analog {

class DelayRequirements {
 public:
  virtual ~DelayRequirements() = default;

  virtual void DelayUs(std::uint32_t delay_us) noexcept = 0;
};

}  // namespace midismith::adc_board::app::analog
