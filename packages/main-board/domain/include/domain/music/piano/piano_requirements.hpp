#pragma once

#include "domain/music/types.hpp"

namespace domain::music::piano {

class PianoRequirements {
 public:
  virtual ~PianoRequirements() = default;

  virtual void PressKey(NoteNumber note, Velocity velocity) noexcept = 0;
  virtual void ReleaseKey(NoteNumber note) noexcept = 0;

  virtual void SetSustain(bool active) noexcept = 0;
  virtual void SetSoft(bool active) noexcept = 0;
  virtual void SetSostenuto(bool active) noexcept = 0;
};

}  // namespace domain::music::piano
