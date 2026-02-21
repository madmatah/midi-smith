#pragma once

#include "domain/music/piano/key_action_requirements.hpp"

namespace domain::music::piano::detail {

class NullKeyActionHandler final : public KeyActionRequirements {
 public:
  void OnNoteOn(domain::music::Velocity velocity) noexcept override {
    (void) velocity;
  }

  void OnNoteOff(domain::music::Velocity release_velocity) noexcept override {
    (void) release_velocity;
  }
};

}  // namespace domain::music::piano::detail
