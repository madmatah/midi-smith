#pragma once

#include "midi/types.hpp"

namespace midismith::adc_board::domain::music::piano {

class KeyActionRequirements {
 public:
  virtual ~KeyActionRequirements() = default;

  virtual void OnNoteOn(midismith::midi::Velocity velocity) noexcept = 0;
  virtual void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept = 0;
};

}  // namespace midismith::adc_board::domain::music::piano
