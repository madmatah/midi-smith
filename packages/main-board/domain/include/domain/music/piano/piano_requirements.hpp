#pragma once

#include "domain/music/types.hpp"

namespace midismith::main_board::domain::music::piano {

class PianoRequirements {
 public:
  virtual ~PianoRequirements() = default;

  virtual void PressKey(midismith::common::domain::music::NoteNumber note,
                        midismith::common::domain::music::Velocity velocity) noexcept = 0;
  virtual void ReleaseKey(midismith::common::domain::music::NoteNumber note) noexcept = 0;

  virtual void SetSustain(bool active) noexcept = 0;
  virtual void SetSoft(bool active) noexcept = 0;
  virtual void SetSostenuto(bool active) noexcept = 0;
};

}  // namespace midismith::main_board::domain::music::piano
