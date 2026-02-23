#pragma once

#include "midi/types.hpp"

namespace midismith::piano_controller {

class PianoRequirements {
 public:
  virtual ~PianoRequirements() = default;

  virtual void PressKey(midismith::midi::NoteNumber note,
                        midismith::midi::Velocity velocity) noexcept = 0;
  virtual void ReleaseKey(midismith::midi::NoteNumber note) noexcept = 0;

  virtual void SetSustain(bool active) noexcept = 0;
  virtual void SetSoft(bool active) noexcept = 0;
  virtual void SetSostenuto(bool active) noexcept = 0;
};

}  // namespace midismith::piano_controller
