#pragma once

#include "midi/types.hpp"

namespace midismith::adc_board::domain::music::piano::velocity {

class VelocityMapperRequirements {
 public:
  virtual ~VelocityMapperRequirements() = default;

  virtual midismith::midi::Velocity Map(float speed_m_per_s) noexcept = 0;
};

}  // namespace midismith::adc_board::domain::music::piano::velocity
