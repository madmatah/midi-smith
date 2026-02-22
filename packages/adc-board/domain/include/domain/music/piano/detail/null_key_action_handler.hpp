#pragma once

#include "domain/music/piano/key_action_requirements.hpp"

namespace midismith::adc_board::domain::music::piano::detail {

class NullKeyActionHandler final : public KeyActionRequirements {
 public:
  void OnNoteOn(midismith::common::domain::music::Velocity velocity) noexcept override {
    (void) velocity;
  }

  void OnNoteOff(midismith::common::domain::music::Velocity release_velocity) noexcept override {
    (void) release_velocity;
  }
};

}  // namespace midismith::adc_board::domain::music::piano::detail
