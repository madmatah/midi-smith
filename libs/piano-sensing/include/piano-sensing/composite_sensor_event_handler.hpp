#pragma once

#include <array>
#include <cstddef>
#include <functional>

#include "piano-sensing/key_action_requirements.hpp"

namespace midismith::piano_sensing {

template <std::size_t kHandlerCount>
class CompositeSensorEventHandler final : public KeyActionRequirements {
 public:
  explicit CompositeSensorEventHandler(
      std::array<std::reference_wrapper<KeyActionRequirements>, kHandlerCount> handlers) noexcept
      : handlers_(handlers) {}

  void OnNoteOn(midismith::midi::Velocity velocity) noexcept override {
    for (KeyActionRequirements& handler : handlers_) {
      handler.OnNoteOn(velocity);
    }
  }

  void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept override {
    for (KeyActionRequirements& handler : handlers_) {
      handler.OnNoteOff(release_velocity);
    }
  }

 private:
  std::array<std::reference_wrapper<KeyActionRequirements>, kHandlerCount> handlers_;
};

}  // namespace midismith::piano_sensing
