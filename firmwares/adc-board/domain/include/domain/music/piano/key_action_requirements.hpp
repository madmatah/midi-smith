#pragma once

#include "domain/music/types.hpp"

namespace midismith::adc_board::domain::music::piano {

class KeyActionRequirements {
 public:
  virtual ~KeyActionRequirements() = default;

  virtual void OnNoteOn(midismith::common::domain::music::Velocity velocity) noexcept = 0;
  virtual void OnNoteOff(midismith::common::domain::music::Velocity release_velocity) noexcept = 0;
};

}  // namespace midismith::adc_board::domain::music::piano
