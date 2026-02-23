#pragma once

#include <cstdint>

#include "midi/midi_controller_requirements.hpp"
#include "piano-controller/piano_requirements.hpp"

namespace midismith::piano_controller {

class MidiPiano : public PianoRequirements {
 public:
  struct Config {
    uint8_t channel;
    uint8_t sustain_cc;
    uint8_t soft_cc;
    uint8_t sostenuto_cc;
  };

  MidiPiano(midismith::midi::MidiControllerRequirements& midi_controller,
            const Config& config) noexcept;

  void PressKey(midismith::midi::NoteNumber note,
                midismith::midi::Velocity velocity) noexcept override;
  void ReleaseKey(midismith::midi::NoteNumber note) noexcept override;

  void SetSustain(bool active) noexcept override;
  void SetSoft(bool active) noexcept override;
  void SetSostenuto(bool active) noexcept override;

 private:
  midismith::midi::MidiControllerRequirements& midi_controller_;
  Config config_;
};

}  // namespace midismith::piano_controller
