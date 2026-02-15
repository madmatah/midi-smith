#pragma once

#include "domain/music/types.hpp"

namespace domain::music::piano {

class KeyActionRequirements {
 public:
  virtual ~KeyActionRequirements() = default;

  virtual void OnNoteOn(domain::music::Velocity velocity) noexcept = 0;
  virtual void OnNoteOff(domain::music::Velocity release_velocity) noexcept = 0;
};

}  // namespace domain::music::piano
