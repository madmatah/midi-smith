#pragma once

#include "piano-sensing/key_action_requirements.hpp"

namespace midismith::piano_sensing::detail {

class NullKeyActionHandler final : public KeyActionRequirements {
 public:
  void OnNoteOn(midismith::midi::Velocity velocity) noexcept override {
    (void) velocity;
  }

  void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept override {
    (void) release_velocity;
  }
};

}  // namespace midismith::piano_sensing::detail
